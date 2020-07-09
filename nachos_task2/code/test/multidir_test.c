#include "syscall.h"

int
main()
{
  char *dir_name = "test_dir";
  char *file_name = "test_dir/Test.txt";
  Make_Dir(dir_name);
  Create(file_name, 3);
  Halt();
}
