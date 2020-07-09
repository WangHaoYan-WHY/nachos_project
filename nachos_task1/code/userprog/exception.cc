// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "main.h"
#include "syscall.h"
#include "ksyscall.h"
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// If you are handling a system call, don't forget to increment the pc
// before returning. (Or else you'll loop making the same system call forever!)
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	is in machine.h.
//----------------------------------------------------------------------

char *Load_Str_From_Memo(int Addr) {
  bool terminated_flag = FALSE;
  char *buffer = new char[128];
  for (int i = 0; i < 128; ++i) {
    if ((buffer[i] = kernel->machine->mainMemory[Addr + i]) == '\0') {
      terminated_flag = true;
      break;
    }
  }
  if (!terminated_flag) {
    delete[] buffer;
    return NULL;
  }
  buffer[127] = '\0';
  return buffer;
}

void
ExceptionHandler(ExceptionType which)
{
    int type = kernel->machine->ReadRegister(2);

    DEBUG(dbgSys, "Received Exception " << which << " type: " << type << "\n");

    switch (which) {
    case SyscallException:
      switch(type) {
      case SC_Halt:
	DEBUG(dbgSys, "Shutdown, initiated by user program.\n");

	SysHalt();

	ASSERTNOTREACHED();
	break;

      case SC_Add:
	DEBUG(dbgSys, "Add " << kernel->machine->ReadRegister(4) << " + " << kernel->machine->ReadRegister(5) << "\n");
	
	/* Process SysAdd Systemcall*/
	int result;
	result = SysAdd(/* int op1 */(int)kernel->machine->ReadRegister(4),
			/* int op2 */(int)kernel->machine->ReadRegister(5));

	DEBUG(dbgSys, "Add returning with " << result << "\n");
	/* Prepare Result */
	kernel->machine->WriteRegister(2, (int)result);
	
	/* Modify return point */
	{
	  /* set previous programm counter (debugging only)*/
	  kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

	  /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
	  kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);
	  
	  /* set next programm counter for brach execution */
	  kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg)+4);
	}

	return;
	
	ASSERTNOTREACHED();

	break;
      case SC_Create:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Create" << "\n";
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
          kernel->file_system->Create(file_name, 0, 1, 1);
        cerr << "Create " << file_name << " Complete" << "\n";
        cerr << "--------------------------------------" << "\n";
        delete[] file_name;
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Open:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Open" << "\n";
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
        OpenFileId open_id = kernel->file_system->new_Open(file_name, 1, 1);
        cerr << "Open " << file_name << " Complete" << "\n";
        cerr << "Open file id:" << open_id << "\n";
        kernel->machine->WriteRegister(2, (int)open_id);
        cerr << "--------------------------------------" << "\n";
        delete[] file_name;
          /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Remove:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Remove" << "\n";
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
        cerr << "file name is: " << file_name << "\n";
        bool flag = kernel->file_system->Remove_File(file_name, 1);
        if (flag == FALSE) {
          cerr << "Can not remove file:" << file_name << "\n";
        }
        else {
          cerr << "Remove " << file_name << " Complete" << "\n";
        }
        kernel->machine->WriteRegister(2, (int)flag);
        cerr << "Write operation complete" << "\n";
        cerr << "--------------------------------------" << "\n";
        delete[] file_name;
          /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Read:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Read Operation" << "\n";
        int num_read = -1;
        char *buffer_r = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
        if (buffer_r == NULL) {
          cerr << "Can not read file:" << "\n";
        }
        else {
          int len_r = kernel->machine->ReadRegister(5);
          OpenFileId file_id_r = kernel->machine->ReadRegister(6);
          cerr << "file id is: " << file_id_r << "\n";
          num_read = 0;
          if (file_id_r == ConsoleInput1) {
            for (int i1 = 0; i1 < len_r; i1++, num_read++) {
            }
          }
          else {
            OpenFile *of_r = kernel->File_Table->convert(file_id_r);
            if (of_r != NULL) {
              cerr << "Start Read" << "\n";
              of_r->Seek(0);

              num_read = of_r->Read(buffer_r, len_r, file_id_r);
              buffer_r[len_r] = '\0';
              cerr << "Read content is:" << buffer_r << "\n";
            }
          }
        }
        delete[] buffer_r;
        cerr << "Read operation complete" << "\n";
        cerr << "--------------------------------------" << "\n";
        kernel->machine->WriteRegister(2, (int)num_read);
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Write:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Write Operation" << "\n";
        int num_write = -1;
        char *buffer_w = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
        if (buffer_w == NULL) {
          cerr << "Can not Write" << "\n";
        }
        else {
          int len_w = kernel->machine->ReadRegister(5);
          OpenFileId file_id_w = kernel->machine->ReadRegister(6);
          num_write = 0;
          if (file_id_w == -1) {
            for (int i2 = 0; i2 < len_w; i2++, num_write++) {
            }
          }
          else {
            OpenFile *of_w = kernel->File_Table->convert(file_id_w);
            of_w->Seek(0);
            if (of_w != NULL) {
              cerr << "Start Write" << "\n";
              num_write = of_w->Write(buffer_w, len_w, file_id_w);
            }
          }
        }
        cerr << "Write operation complete" << "\n";
        delete[] buffer_w;
        cerr << "--------------------------------------" << "\n";
        kernel->machine->WriteRegister(2, (int)num_write);
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Close:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Close Operation" << "\n";
        OpenFileId close_file_id = kernel->machine->ReadRegister(4);
        cerr << "Open File Id is: " << close_file_id << "\n";
        kernel->File_Table->remove(close_file_id);
        kernel->machine->WriteRegister(2, (int)close_file_id);
        cerr << "--------------------------------------" << "\n";
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Makedir:
      {
        cerr << "--------------------------------------" << "\n";
        cerr << "Make directory Operation" << "\n";
        char *dir_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4));
        int make_dir_res = -1;
        if (dir_name == NULL) {
          kernel->machine->WriteRegister(2, (int)make_dir_res);
        }
        else if (!kernel->fileSystem->Make_Dir(dir_name, 0, 1)) {
          delete[] dir_name;
          kernel->machine->WriteRegister(2, (int)make_dir_res);
        }
        else {
          delete[] dir_name;
          make_dir_res = 1;
          kernel->machine->WriteRegister(2, (int)make_dir_res);
        }
        cerr << "--------------------------------------" << "\n";
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        return;
        ASSERTNOTREACHED();
        break;
      }

      case SC_Exit:
      {
        kernel->currentThread->Finish();
      }
      default:
	cerr << "Unexpected system call " << type << "\n";
	break;
      }
      break;
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
    }
    ASSERTNOTREACHED();
}
