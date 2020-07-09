#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  OpenFileId id;
  Create(name);
  id = Open(name);
  Close(id);
  Remove(name);
  Halt();
}
