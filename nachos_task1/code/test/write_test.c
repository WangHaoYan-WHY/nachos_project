#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  char *buffer = "test";
  OpenFileId id;
  Create(name);
  id = Open(name);
  Write(buffer, 4, id);
  Halt();
}
