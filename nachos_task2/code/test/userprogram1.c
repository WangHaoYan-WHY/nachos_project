#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  Create(name);
  return 0;
  Halt();
}