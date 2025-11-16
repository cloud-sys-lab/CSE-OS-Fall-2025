#define PageSize	4096
#define NumPages	128

/* a large array, 2 times the size of physical memory */
#define ArraySize (2*NumPages*PageSize)
char sparse[ArraySize];

int main(void) {
  int i;
  for (i = 0; i < ArraySize; i += PageSize * 10) {
    sparse[i] = 1;
  }
  return 0;
}
