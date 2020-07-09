#include "syscall.h"

int
main()
{
  char *name = "Test.txt";
  char *content = "test";
  char buffer[1000];
  OpenFileId id;
  Create(name);
  id = Open(name);
  Write(content, 4, id);
  Read(buffer, 4, id);
  Halt();
}
