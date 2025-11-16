#include <types.h>
#include <lib.h>
#include <thread.h>

int sys__exit(int code) {
  kprintf("%d \n", code);
  thread_exit();
  return 0;
}
