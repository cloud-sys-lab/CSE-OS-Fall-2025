#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Create an address space, also creating its corresponding page table
 */
struct addrspace * as_create(void) {
  struct addrspace *as = kmalloc(sizeof(struct addrspace));
  if (as==NULL) {
    return NULL;
  }

  as->as_vbase1 = 0;
  as->as_npages1 = 0;
  as->as_vbase2 = 0;
  as->as_npages2 = 0;
  as->as_npages_s = 0;

  struct page_table * pt = pt_create();
  if (pt == NULL) {
    kfree(as);
    return NULL;
  }
        
  as->pt = pt;
  return as;
}

/*
 * Destroy an address space, also freeing all its allocated memory.
 */
void as_destroy(struct addrspace *as) {
  pt_destroy(as->pt);
  kfree(as);
}

/*
 * Activate an address space
 */
void as_activate(struct addrspace *as) {
  flush_tlb();
}

/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 *
 * This is mostly copied from dumbvm.c
 */
int as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
                     int readable, int writeable, int executable) {
  size_t npages; 

  /* Align the region. First, the base... */
  sz += vaddr & ~(vaddr_t)PAGE_FRAME;
  vaddr &= PAGE_FRAME;

  /* ...and now the length. */
  sz = (sz + PAGE_SIZE - 1) & PAGE_FRAME;

  npages = sz / PAGE_SIZE;

  /* We don't use these - all pages are read-write */
  (void)readable;
  (void)writeable;
  (void)executable;

  if (as->as_npages1 == 0) {
    as->as_vbase1 = vaddr;
    as->as_npages1 = npages;
    return 0;
  }

  if (as->as_npages2 == 0) {
    as->as_vbase2 = vaddr;
    as->as_npages2 = npages;
    return 0;
  }

  /*
   * Support for more than two regions is not available.
   */
  kprintf("addrspace: Warning: too many regions\n");
  return EUNIMP;
}

/*
 * Check if the given virtual address is assigned in this address space.
 * This means it is in one of the regions allocated with as_define_region
 * or is within the stack.
 */
int as_check_access(struct addrspace *as, vaddr_t access) {
  vaddr_t vbase1 = as->as_vbase1;
  vaddr_t vtop1 = vbase1 + as->as_npages1 * PAGE_SIZE;
  vaddr_t vbase2 = as->as_vbase2;
  vaddr_t vtop2 = vbase2 + as->as_npages2 * PAGE_SIZE;
  vaddr_t stackbase = USERSTACK - as->as_npages_s * PAGE_SIZE;
  vaddr_t stacktop = USERSTACK;

  // TODO: check read/write/execute permissions
  
  if (access >= vbase1 && access < vtop1) {
    return 0;
  } else if (access >= vbase2 && access < vtop2) {
    return 0;
  } else if (access >= stackbase && access < stacktop) {
    return 0;
  } else {
    return EFAULT;
  }
}

int as_prepare_load(struct addrspace *as) {
  /*
   * Unneeded.
   */

  (void)as;
  return 0;
}

int as_complete_load(struct addrspace *as) {
  /*
   * Unneeded.
   */

  (void)as;
  return 0;
}

/*
 * This seems almost vestigial, as the stack is always the same size and in the same place.
 * Changing this behavior would require changes to the threading code.
 */
int as_define_stack(struct addrspace *as, vaddr_t *stackptr) {
  as->as_npages_s = STACKPAGES;

  /* Initial user-level stack pointer */
  *stackptr = USERSTACK;
	
  return 0;
}

int as_copy(struct addrspace *old, struct addrspace **ret) {
  struct addrspace *newas;

  newas = as_create();
  if (newas==NULL) {
    return ENOMEM;
  }

  /*
   * Unimplemented. There are no examples, so semantics are unclear.
   */

  (void)old;
	
  *ret = newas;
  return EUNIMP;
}
