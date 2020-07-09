#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  Create(name);
  Open(name);
  Remove(name);
  return 0;
  Halt();
}
