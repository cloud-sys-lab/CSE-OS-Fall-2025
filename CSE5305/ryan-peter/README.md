# VM subsystem

We have implemented a VM subsystem for OS161. 
We support allocating and deallocating variable-size blocks of physical memory. 
Virtual memory is implemented with a two-level page table.

# Building & Testing

The system can be built and executed with the ordinary OS161 process.
Note that the configuration file you should use is kern/conf/proj.

We have implemented two simple test programs. These are testbin/vm_t1 and testbin/vm_t2.
Both are very primitive, doing almost nothing as we don't support most system calls. 
However, vm_t2 does demonstrate our support for sparse allocations.

You can also test with testbin/sparse, which will complete successfully despite the unimplemented system calls.
