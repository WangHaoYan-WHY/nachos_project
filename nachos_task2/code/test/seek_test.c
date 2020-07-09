#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  char *buffer;
  OpenFileId id;
  Create(name, 3);
  id = Open(name, 3);
  Seek(0, id);
  Halt();
}
