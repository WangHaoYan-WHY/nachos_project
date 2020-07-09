#include "syscall.h"
#define NUM_ARGS_shell 11
#define ARG_LEN_shell 60
int
main()
{
    char *prompt = "--", buffer[ARG_LEN_shell], ch, args[NUM_ARGS_shell][ARG_LEN_shell], *argv[NUM_ARGS_shell];
    SpaceId newProc;
    OpenFileId input = 0;
    OpenFileId output = 1;
    int index, arg_index, current_char;
    Write("--", 2, output);
    while (1) {
      for (index = 1; index < ARG_LEN_shell; index++) {
        buffer[index] = '\0';
      }
      Write(prompt, 2, output);
      index = 0;
      do {
        Read(&buffer[index], 1, input);
      } while (buffer[index++] != '\n');
      buffer[--index] = '\0';
      if (index > 0) {
        for (arg_index = 0; arg_index < NUM_ARGS_shell; ++arg_index) {
          args[arg_index][0] = '\0';
        }
        for (index = 0, arg_index = 0, current_char = 0; index < ARG_LEN_shell; index++) {
          if (buffer[index] = ' ') {
            args[arg_index++][current_char] = '\0';
            current_char = 0;
          }
          else {
            args[arg_index][current_char] = buffer[index];
          }
        }
        for (index = 0; index < arg_index; index++) {
          argv[index] = args[index];
        }
        argv[arg_index + 1] = (char*)0;
        newProc = Exec(argv[0], argv, 0);
        Join(newProc);
      }
    }
    return 0;
}

