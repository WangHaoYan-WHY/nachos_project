#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  OpenFileId id;
  Create(name, 3);
  id = Open(name, 3);
  Close(id);
  Remove(name);
  Halt();
}
