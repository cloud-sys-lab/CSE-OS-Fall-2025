#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <curthread.h>
#include <addrspace.h>
#include <vm.h>
#include <machine/spl.h>
#include <machine/tlb.h>

/* always have 48k of user stack */
#define STACKPAGES    12

#define ROUND_DOWN(addr) (((addr) / PAGE_SIZE) * PAGE_SIZE)
#define ROUND_UP(addr)   (ROUND_DOWN(addr + PAGE_SIZE - 1))
#define N_PAGES(bytes)   (ROUND_UP(bytes) / PAGE_SIZE)
#define PAGE_IDX(addr)   (ROUND_DOWN(addr) / PAGE_SIZE)

/*
 * A very simple inverted page table. Each entry just
 * tracks the size of the contiguous allocation starting
 * at the corresponding page. Each entry after the first
 * in each allocation has a length of zero.
 *
 * The first entry corresponds to the page starting at address lo,
 * the second entry to the second page, and so on.
 */
struct pmem_info {
  int alloc_len;
};
struct pmem_info *ipt;

// The total number of pages managed by this subsystem
int system_pages;
// The number of pages that are available for allocation
int avail_pages;
// Zero if the system is not yet bootstrapped; 1 otherwise
int bootstrapped;

// Low address of the memory managed by this system
// We manage all memory from lo to the highest physical address
paddr_t lo;

/*
 * Bootstrap the VM memory subsystem, setting up global variables
 * and setting up the inverted page table
 */
void vm_bootstrap(void) {
  int spl = splhigh();
  paddr_t hi;
  ram_getsize(&lo, &hi);
  system_pages = N_PAGES(hi - lo);
  avail_pages = system_pages;

  // Write our inverted page table into memory
  ipt = (struct pmem_info *)PADDR_TO_KVADDR(lo);
  int i;
  for (i = 0; i < system_pages; i++) {
    ipt[i].alloc_len = 0;
  }
  bootstrapped = 1;

  // Reserve the memory used by the inverted page table
  // Otherwise it would get clobbered.
  alloc_kpages(N_PAGES(system_pages * sizeof(struct pmem_info)));

  splx(spl);
}

/*
 * Allocate a contiguous region of npages
 *
 * Linearly scans through the inverted page table until it
 * finds a region that is large enough. This makes allocation
 * a linear-time operation.
 *
 * Returns the physical addrss of the start of the allocated region.
 */
static paddr_t alloc_ppages(int npages) {
  assert(curspl>0);
  int i = 0;
  int len = 0;
  // Find a contiguous region of memory large enough for this allocation
  while (len < npages && i < system_pages) {
    if (ipt[i].alloc_len > 0) {
      // The page we are considering is the start of an allocated region.
      // Skip that region, and set length of the found region to zero.
      i += ipt[i].alloc_len;
      len = 0;
    } else {
      // The page is not currently allocated.
      // We should continue and check the next one.
      i += 1;
      len += 1;
    }
  }

  paddr_t ret;
  if (i == system_pages) {
    // We got to the end of the IPT without finding a region.
    ret = (paddr_t)NULL;
  } else {
    // We found a suitable region. Mark this allocation in the table...
    ipt[i - npages].alloc_len = npages;
    // and return the paddr corresponding to this entry
    // (note that +lo, as the memory we manage starts at lo)
    ret = (paddr_t)((i - npages) * PAGE_SIZE + lo);
  }

  avail_pages -= npages;
  return ret;
}

/*
 * Allocate npages for kernel use.
 *
 * Early in boot this uses ram_stealmem.
 *
 * Any memory allocated before vm_bootstrap is called
 * CANNOT be freed, so don't try to free it with free_kpages
 */
vaddr_t alloc_kpages(int npages) {
  int spl = splhigh();
  paddr_t ret;
  if(bootstrapped) {
    // We are up, so use the IPT
    ret = alloc_ppages(npages);
  } else {
    ret = ram_stealmem(npages);
  }
  splx(spl);
  return PADDR_TO_KVADDR(ret);
}

/*
 * Free physical pages allocated with alloc_ppages
 *
 * Will free the entire region that was allocated, not just the page pointed to.
 */
static void free_ppages(paddr_t addr) {
  int page = PAGE_IDX(addr - lo);

  avail_pages += ipt[page].alloc_len;
  ipt[page].alloc_len = 0;
}

/*
 * Free kernel pages allocated with alloc_kpages
 *
 * Will free the entire region, not just the page pointed to.
 *
 * Do NOT call with any memory allocated before vm_bootstrap was called.
 */
void free_kpages(vaddr_t addr) {
  free_ppages(KVADDR_TO_PADDR(addr));
}

/*
 * Create a new internal node for a page table.
 *
 * ALlocates the memory and initializes all the pages it points to to NULL
 */
static struct pt_node * pt_node_create(void) {
  struct pt_node * node = (struct pt_node *)alloc_kpages(N_PAGES(sizeof(struct pt_node)));

  if (node == NULL) {
    return NULL;
  }

  int i;
  for (i = 0; i < PT_FANOUT; i++) {
    node->children[i].paddr = 0;
  }
  return node;
}

/*
 * Create a page table.
 *
 * Allocates the memory for the top level of the page table, but NOT for the second-level nodes.
 * Those are created lazily when they are needed.
 */
struct page_table * pt_create(void) {
  struct page_table * pt = (struct page_table *)alloc_kpages(N_PAGES(sizeof(struct page_table)));

  if (pt == NULL) {
    return NULL;
  }

  int i;
  for (i = 0; i < PT_FANOUT; i++) {
    pt->children[i] = NULL;
  }
  return pt;
}

/*
 * Destroy a page table node and deallocate all the memory it points to.
 */
static void pt_node_destroy(struct pt_node *node) {
  int i;
  for (i = 0; i < PT_FANOUT; i++) {
    if (node->children[i].paddr) {
      free_ppages(node->children[i].paddr);
    }
  }
  free_kpages((vaddr_t)node);
}

/*
 * Destroy a page table, destroy all the second-level nodes,
 * and deallocate all physical pages they point to.
 */
void pt_destroy(struct page_table *pt) {
  int i;
  for (i = 0; i < PT_FANOUT; i++) {
    if (pt->children[i] != NULL) {
      pt_node_destroy(pt->children[i]);
    }
  }
  free_kpages((vaddr_t)pt);
}

/*
 * Walk the page table to find the physical page pointed to by the fault address.
 * If the second-level node is not allocated, allocate it.
 * If the virtual address has no physical page allocated, allocate one.
 * Returns the paddr of the page if it exists or can be allocated,
 * or NULL on out of memory.
 */
static paddr_t walk_pt(struct page_table *pt, vaddr_t faultaddress) {
  //  int lo_mask = 0x00000fff;
  //  int lo = fault_address & low_mask;

  // Get the middle-order bits, which are the index used for the second level.
  int mid_mask = 0x000003ff;
  int mid = (faultaddress >> 12) & mid_mask;

  // Get the high-order bits, which are the index used for the first level.
  int hi_mask = 0x000003ff;
  int hi = (faultaddress >> 22) & hi_mask;

  if (pt->children[hi] == NULL) {
    // There is no second-level node corresponding to the high-order part of the address.
    // Need to allocate an intermediate node
    struct pt_node *node = pt_node_create();
    if (node == NULL) {
      // out of memory
      return (paddr_t)NULL;
    }
    pt->children[hi] = node;
  }
  
  if (pt->children[hi]->children[mid].paddr == (paddr_t)NULL) {
    // The second-level node has no page for this vaddr
    // Need to allocate a backing page
    paddr_t new_page = alloc_ppages(1);
    if (new_page == (paddr_t)NULL) {
      // out of memory
      return (paddr_t)NULL;
    }
    // Zero the new page to avoid leaking data
    int i = 0;
    for (i = 0; i < PAGE_SIZE; i++) {
      *(char*)PADDR_TO_KVADDR(new_page + i) = 0;
    }
    pt->children[hi]->children[mid].paddr = new_page;
  }

  return pt->children[hi]->children[mid].paddr;
}

/*
 * Handle a virtual memory fault.
 *
 * Will check if the faulting address is in a virtual address region the process has mapped.
 * If it is, will find or allocate a corresponding page of memory.
 * Returns EFAULT if the access is illega, ENOMEM on out of memory.
 *
 * TLB entries are inserted to a random position in the TLB.
 */
int vm_fault(int faulttype, vaddr_t faultaddress) {
  int spl = splhigh();
  faultaddress = ROUND_DOWN(faultaddress);

  // This is largely unnecessary as all supported faults are handled the same
  // Kept for completeness
  switch (faulttype) {
  case VM_FAULT_READONLY:
    /* We always create pages read-write, so we can't get this */
    panic("vm: got VM_FAULT_READONLY\n");
  case VM_FAULT_READ:
  case VM_FAULT_WRITE:
    break;
  default:
    splx(spl);
    return EINVAL;
  }

  struct addrspace *as = curthread->t_vmspace;
  if (as == NULL) {
    // kprintf("Address space is bad\n");
    /*
     * No address space set up. This is probably a kernel
     * fault early in boot. Return EFAULT so as to panic
     * instead of getting into an infinite faulting loop.
     */
    splx(spl);
    return EFAULT;
  }

  if (as_check_access(as, faultaddress)) {
    // kprintf("Access to unmapped memory\n");
    /*
     * This virtual memory address is not associated with
     * any region defined in the address space.
     */
    splx(spl);
    return EFAULT;
  }

  paddr_t paddr = walk_pt(as->pt, faultaddress);
  if (paddr == (paddr_t)NULL) {
    // kprintf("out of memory\n");
    // The page was not yet allocated a physical page, and there are none available.
    splx(spl);
    return ENOMEM;
  }
  // Store the found physical address in the TLB
  vaddr_t ehi = faultaddress;
  paddr_t elo = paddr | TLBLO_DIRTY | TLBLO_VALID;
  TLB_Random(ehi, elo);
  
  splx(spl);
  return 0;
}

/*
 * Flush all entries in the TLB
 * Needed on every context switch
 */
void flush_tlb() {
  int spl = splhigh();
  int i;
  for (i = 0; i < NUM_TLB; i++) {
    TLB_Write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
  }

  splx(spl);
}
