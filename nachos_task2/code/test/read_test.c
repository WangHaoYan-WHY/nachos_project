#include "syscall.h"

int
main()
{
  char *name = "Test.txt", *buffer;
  OpenFileId id;
  Create(name, 3);
  id = Open(name, 3);
  Write("test", 4, id);
  Read(buffer, 4, id);
  Halt();
}
