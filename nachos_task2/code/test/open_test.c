#include "syscall.h"

int
main(int argc, char** argv)
{
  char *name = "Test.txt";
  Create(name, 3);
  Open(name, 3);
  Remove(name);
  Halt();
}