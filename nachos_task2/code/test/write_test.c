#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  char *buffer = "test";
  char *buffer1;
  OpenFileId id;
  Create(name, 3);
  id = Open(name, 3);
  Write(buffer, 4, id);
  Read(buffer1, 4, id);
  Halt();
}
