#include "syscall.h"

int
main()
{
  char *name = "test_dir";
  Make_Dir(name);
  Halt();
}
