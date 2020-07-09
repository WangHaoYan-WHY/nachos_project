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
#include "thread.h"
#include "synchconsole.h"
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
// fork function which restore the state and run the user program
void usrfork(int which)
{
  kernel->currentThread->RestoreUserState();
  kernel->currentThread->space->RestoreState();
  kernel->machine->Run();
}

// user program execution
void usrexec(Thread *cur_t)
{
  cur_t->space->Execute();
}

char *Load_Str_From_Memo(int Addr, int len) {
  bool terminated_flag = FALSE;
  char *buffer = new char[128];
  int i = 0;
  for (i = 0; i < 128; ++i) {
    //if((buffer[i] = kernel->machine->ReadMem(Addr + i, 1, (int*)(buffer + i))) == '\0') {
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

// execute function
void Exec_func(int args) {
  char** exec_fork_strings = (char **)args; // the args for execute
  kernel->currentThread->space->InitRegisters(); // initialize the reigister
  kernel->currentThread->space->RestoreState(); // restore the state, load the page table
  int exec_fork_argc = 0; // number of the execute argument
  int exec_fork_store = kernel->machine->ReadRegister(StackReg); // copy of the stak pointer
  if (exec_fork_strings != NULL) {
    for (int i = 0; exec_fork_strings[i] != 0; i++, exec_fork_argc++);
    int ef_ad[exec_fork_argc]; // hold the address
    for (int i = 0; exec_fork_strings[i] != 0; i++) {
      int ef_len = strlen(exec_fork_strings[i]) + 1; // get the string len of the arguments
      exec_fork_store -= ef_len;
      for (int j = 0; j < ef_len; j++) {
        kernel->machine->WriteMem(exec_fork_store + j, 1, exec_fork_strings[i][j]); // write into the memory
      }
      ef_ad[i] = exec_fork_store; // store the stack pointer
    }
    exec_fork_store = exec_fork_store & ~3; // get the offset
    exec_fork_store -= sizeof(int) *exec_fork_argc;
    for (int i = 0; i < exec_fork_argc; i++) {
      kernel->machine->WriteMem(exec_fork_store + i * 4, 4, WordToMachine((unsigned int) ef_ad[i])); // write into memory
    }
  }
  kernel->machine->WriteRegister(4, exec_fork_argc); // write the number of execute argument into register 4
  kernel->machine->WriteRegister(5, exec_fork_store); // write the copy of the stack pointer into the register 5
  kernel->machine->WriteRegister(StackReg, exec_fork_store - 8); // write into the stack register
  kernel->machine->Run();
}

// execute
int Execute_method() {
  char *exec_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4), 128);
  int exec_base = kernel->machine->ReadRegister(5);
  int exec_share = kernel->machine->ReadRegister(6);
  if (exec_name == NULL) {
    return -1;
  }
  char **arg_strings = NULL;
  if (exec_base != 0) {
    arg_strings = new char*[12];
    // get the args as stored into the strings
    for (int i = 0; i < exec_share; i++) {
      int* exec_store = new int();
      kernel->machine->ReadMem(exec_base + i * 4, 4, exec_store);
      arg_strings[i] = Load_Str_From_Memo(*exec_store, 128);
    }
    arg_strings[exec_share] = (char*)NULL;
  }
  Thread *exec_thread = new Thread("exec-thread");
  exec_thread->parent = kernel->currentThread;
  kernel->currentThread->child_threads_ids->Append(exec_thread->thread_id); //append the thread id into childs ids
  exec_thread->space = new AddrSpace();
  kernel->swap_space_counter = 0; // set the initialize swap space counter as 0
  exec_thread->space->Load(exec_name); // load the execute file into the space
  exec_thread->space->p_table = new Per_Process_OpenFile_Table();
  exec_thread->space->work_dir_name = kernel->currentThread->space->work_dir_name; // same work directory name
  exec_thread->space->work_dir_sector = kernel->currentThread->space->work_dir_sector; // same work directory sector
  exec_thread->Fork((VoidFunctionPtr)(Exec_func), (void*)arg_strings); // do fork method
  return exec_thread->thread_id;
}

// fork function
ThreadId Sys_Fork(int input) {
  Thread *new_child = new Thread("child_Thread");
  new_child->parent = kernel->currentThread;
  kernel->currentThread->child_threads->Append(new_child);
  new_child->Fork((VoidFunctionPtr)usrfork, (void*)new_child->thread_id);
  return new_child->thread_id;
}

// do exit
void Exit_P(ThreadId t_id) {
  if (kernel->currentThread->thread_id == t_id) {
    Thread *parent_thread = kernel->currentThread->parent;
    // check the parent is finished or not
    if (parent_thread != NULL && !kernel->scheduler->finished_threads->IsInList(parent_thread->thread_id)) {
      // check the parent thread is in ready list or not
      if (!kernel->scheduler->readyList->IsInList(kernel->currentThread->parent)) {
        IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
        kernel->scheduler->ReadyToRun(parent_thread); // set the parent thread ready to run
        (void)kernel->interrupt->SetLevel(oldLevel);
      }
      parent_thread->finished_childs->Append(kernel->currentThread->thread_id); // set into finished threads
      parent_thread->child_threads->Remove(kernel->currentThread); // remove from the child threads
    }
    kernel->currentThread->Finish(); // finish current threads
  }
}

// join function
void join_Func(ThreadId child_ID) {
  if (kernel->currentThread->finished_childs->IsInList(child_ID)) {
    return;
  }
  ListIterator<Thread*> *iter_childs = new ListIterator<Thread*>(kernel->currentThread->child_threads);
  for (; !iter_childs->IsDone(); iter_childs->Next()) { // iterate the childs lists
    Thread *store = iter_childs->Item();
    if (store->thread_id == child_ID) {
      IntStatus oldLevel = kernel->interrupt->SetLevel(IntOff);
      kernel->currentThread->Sleep(FALSE); // set current Thread to fall sleep
      (void)kernel->interrupt->SetLevel(oldLevel);
      return;
    }
  }
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
        cerr << "Create" << "\n";
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4), 128); // get file name
        int protection_num = kernel->machine->ReadRegister(5); // get protection number
        kernel->fileSystem->Create(file_name, 0, protection_num, kernel->currentThread->space->work_dir_sector);
        cerr << "Create " << file_name << " Complete" << "\n";
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
        cerr << "Open" << "\n";
        // load the file name from register 4 which is char*
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4), 128);
        int open_mode = kernel->machine->ReadRegister(5);
        // use new_open method to open the file
        OpenFileId open_id = kernel->fileSystem->new_Open(file_name, open_mode, kernel->currentThread->space->work_dir_sector);
        cerr << "Open " << file_name << " Complete" << "\n";
        cerr << "Open file id:" << open_id << "\n";
        kernel->machine->WriteRegister(2, (int)open_id); // write into register 2
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
        cerr << "Remove" << "\n";
        char *file_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4), 128); // get file names
        bool flag = kernel->fileSystem->Remove_File(file_name, 1); // remove the file
        if (flag == FALSE) {
          cerr << "Can not remove file:" << file_name << "\n";
        }
        else {
          cerr << "Remove " << file_name << " Complete" << "\n";
        }
        kernel->machine->WriteRegister(2, (int)flag); // write the flag into register
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
        cerr << "Read Operation" << "\n";
        int num_read = -1;
        char *buffer_r = new char[128];
        int addr_read = kernel->machine->ReadRegister(4);
        if (buffer_r == NULL) {
          cerr << "Can not read file:" << "\n";
        }
        else {
          int len_r = kernel->machine->ReadRegister(5);
          OpenFileId file_id_r = kernel->machine->ReadRegister(6);
          cerr << "file id is: " << file_id_r << "\n";
          num_read = 0;
          if (file_id_r == ConsoleInput1) {
            for (int i_r = 0; i_r < len_r; i_r++, num_read++) {
              buffer_r[i_r] = kernel->synchConsoleIn->GetChar();
            }
          }
          else {
            OpenFile *of_r = kernel->currentThread->space->p_table->convert(file_id_r); // get the corresponding open file of file id
            if (of_r != NULL) {
              cerr << "Start Read" << "\n";
              of_r->Seek(0);
              num_read = of_r->Read(buffer_r, len_r, file_id_r); // read the content from file id
              buffer_r[len_r] = '\0';
              cerr << "Read content is:" << buffer_r << "\n";
              for (int i_rb = 0; i_rb <= len_r; i_rb++) {
                kernel->machine->WriteRegister(addr_read + i_rb, buffer_r[i_rb]); // read into the read address
              }
            }
          }
        }
        delete[] buffer_r;
        cerr << "Read operation complete" << "\n";
        kernel->machine->WriteRegister(2, (int)num_read); // store OpenFileId into the register 2
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
        cerr << "Write Operation" << "\n";
        int num_write = -1;
        int len_w = kernel->machine->ReadRegister(5); // get te length to be written
        char *buffer_w = Load_Str_From_Memo(kernel->machine->ReadRegister(4), len_w); // read the content to be written from register 4
        if (buffer_w == NULL) {
          cerr << "Can not Write" << "\n";
        }
        else {
          len_w = kernel->machine->ReadRegister(5);
          OpenFileId file_id_w = kernel->machine->ReadRegister(6); // file id
          num_write = 0;
          if (file_id_w == ConsoleOutput1) { // console output
            char *write_char = buffer_w;
            buffer_w[len_w] = '\0';
            cerr << "Write content is: " << buffer_w << "\n";
            while (len_w > 0) {
              kernel->synchConsoleOut->PutChar(*write_char); // do put char operation
              *write_char++;
              len_w--;
            }
          }
          else {
            cerr << "Openfile id is: "  << file_id_w  << "\n";
            OpenFile *of_w = kernel->currentThread->space->p_table->convert(file_id_w); // get the corresponding open file
            of_w->Seek(0);
            if (of_w != NULL) {
              cerr << "Start Write" << "\n";
              cerr << "Write content is: " << buffer_w << "\n";
              fflush(stdin);
              num_write = of_w->WriteAt(buffer_w, len_w, file_id_w); // write content from buffer into the file
            }
          }
        }
        cerr << "Write operation complete" << "\n";
        delete[] buffer_w;
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
        cerr << "Close Operation" << "\n";
        OpenFileId close_file_id = kernel->machine->ReadRegister(4); // read to be closed open file id
        cerr << "Open File Id is: " << close_file_id << "\n";
        kernel->File_Table->remove(close_file_id); // remove the close file id
        kernel->machine->WriteRegister(2, (int)close_file_id); // write the close file id into the register 2
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
        cerr << "Make directory Operation" << "\n";
        char *dir_name = Load_Str_From_Memo(kernel->machine->ReadRegister(4), 128);
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
      case SC_Exec:
      {
        cerr << "Exec Operation" << "\n";
        Execute_method();
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
      // system call SC_ThreadFork
      case SC_ThreadFork:
      {
        //cerr << "-------------------------------------" << "\n";
        //cerr << "call SC_ThreadFork call:" << "\n";
        //cerr << "-------------------------------------" << "\n";
        int addrF = kernel->machine->ReadRegister(4); // read the address stored in register
        Thread *t = new Thread("newThread"); // new thread
        t->space = new AddrSpace(*kernel->currentThread->space); // use copy constructor to initialize the space of thread
        t->SaveUserState(); // save the user state 
        t->set_Register(PCReg, addrF); // set the PC register as addrF
        t->set_Register(NextPCReg, addrF + 4); // set next PC register as next instruction's address
        t->Fork((VoidFunctionPtr)Sys_Fork, (void *)0); // do fork
        kernel->machine->WriteRegister(2, (int)t->space->get_space_id()); // store the return result in register 2
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
      case SC_Console_Read:
      {
        //cerr << "-------------------------------------" << "\n";
        //cerr << "call SC_Console_Read system call:" << "\n";
        //cerr << "-------------------------------------" << "\n";
        int numR = kernel->machine->ReadRegister(5); // read the number to be read from register 5
        int addrR = kernel->machine->ReadRegister(4); // read starting address from register 4
        int resultR = SysRead(addrR, numR); // call SysRead which handle the read operation
        kernel->machine->WriteRegister(2, (int)resultR);
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
      case SC_Console_Write:
      {
        //cerr << "-------------------------------------" << "\n";
        //cerr << "call SC_Console_Write system call:" << "\n";
        //cerr << "-------------------------------------" << "\n";
        int numW = kernel->machine->ReadRegister(5); // read the number to be written from register 5
        int addrW = kernel->machine->ReadRegister(4); // read starting address from register 4
        int resultW = SysWrite(addrW, numW); // call SysWrite which handle write operation
        kernel->machine->WriteRegister(2, (int)resultW);
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
        //cerr << "-------------------------------------" << "\n";
        //cerr << "call SC_Exit system call:" << "\n";
        //cerr << "-------------------------------------" << "\n";
        AddrSpace *store_space = kernel->currentThread->space; // thread's space
        //cerr << "-------------------------------------" << "\n";
        //cerr << "Current page fault number is: " << kernel->stats->numPageFaults << "\n";
        //cerr << "-------------------------------------" << "\n";
        ThreadId exit_id = kernel->machine->ReadRegister(4);
        if (exit_id != 0) {
          Exit_P(exit_id);
        }
        else {
          kernel->currentThread->Finish(); // finish the current thread
        }
        /* Modify return point */
        {
          /* set previous programm counter (debugging only)*/
          kernel->machine->WriteRegister(PrevPCReg, kernel->machine->ReadRegister(PCReg));

          /* set programm counter to next instruction (all Instructions are 4 byte wide)*/
          kernel->machine->WriteRegister(PCReg, kernel->machine->ReadRegister(PCReg) + 4);

          /* set next programm counter for brach execution */
          kernel->machine->WriteRegister(NextPCReg, kernel->machine->ReadRegister(PCReg) + 4);
        }
        delete store_space; // delet the thread's space
        return;
        ASSERTNOTREACHED();
        break;
      }
      case SC_Join:
      {
        ThreadId child_ID_WP = (int)kernel->machine->ReadRegister(4);
        cerr << "Join thread id is: " << child_ID_WP << "\n";
        join_Func(child_ID_WP);
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
      case SC_seek:
      {
        cerr << "Seek Operation" << "\n";
        int start_seek = kernel->machine->ReadRegister(4);
        OpenFileId id_seek = kernel->machine->ReadRegister(5);
        OpenFile *of_seek = kernel->currentThread->space->p_table->convert(id_seek); // get the corresponding open file of file id
        if (of_seek != NULL) {
          of_seek->Seek(start_seek, kernel->File_Table->GetName(id_seek));
          kernel->machine->WriteRegister(2, (int)start_seek); // store start seek number into the register 2
        }
        else {
          cerr << "file name does not exist" << "\n";
          kernel->machine->WriteRegister(2, -1); // store start seek number into the register 2
        }
        cerr << "Seek operation complete" << "\n";
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
      case SC_change_dir:
      {
        ThreadId child_ID_WP = (int)kernel->machine->ReadRegister(4);
        cerr << "Join thread id is: " << child_ID_WP << "\n";
        join_Func(child_ID_WP);
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
      default:
        cerr << "Unexpected system call " << type << "\n";
        break;
    }

    case PageFaultException:
    {
      //cerr << "-------------------------------------" << "\n";
      //cerr << "call PageFaultException system call:" << "\n";
      kernel->stats->numPageFaults++;
      int page_Fault_addr = kernel->machine->ReadRegister(BadVAddrReg); // page fault's address stored in BadAddrReg
      int page_Fault_virtual = page_Fault_addr / PageSize; // calculate virtual page of the page fault
      int Page_Fault_physical = kernel->record_free_Map->FindAndSet(); // find free physical page in recrod_free_Map which is bitmap
      TranslationEntry* page_Table = kernel->currentThread->space->get_Page_Table(); // page table of current thread's space
      TranslationEntry *page_Entry = &page_Table[page_Fault_virtual]; // get corresponding translation entry according to virtual page of page fault
      if (Page_Fault_physical != -1) { // do not need swap pages
        //cerr << "no swap:" << "\n";
        //cerr << "Insert page entry's physical page is: " << Page_Fault_physical << "\n";
        //cerr << "Insert page entry's virtual page is: " << page_Entry->virtualPage << "\n";
        page_Entry->physicalPage = Page_Fault_physical; // set physical page as the found physical page in recrod_free_Map
        page_Entry->valid = TRUE; // set current page as valid
        kernel->swap_space->ReadAt(
          &(kernel->machine->mainMemory[Page_Fault_physical * PageSize]),
          PageSize, page_Entry->virtualPage * PageSize); // read a portion of the file into main memory
        if (kernel->random_flag == 1) {
          kernel->random_list->set(page_Entry); // random data structure
        }
        else {
          kernel->LRU_Entries->set(page_Entry, page_Entry); // insert the page entry into the LRU data structure
        }
      }
      else { // need swap pages
        //cerr << "pages swap:" << "\n";
        TranslationEntry* out_entry = NULL;
        if (kernel->random_flag == 1) {
          out_entry = kernel->random_list->remove(); //remove from random structure
        }
        else {
          out_entry = kernel->LRU_Entries->removeFront(); // remove LRU data structure
        }
        int physical_num = out_entry->physicalPage; // physical page of the least recent used one
        int virtual_num = out_entry->virtualPage; // virtual page of the least recent used one
        kernel->swap_space->WriteAt(
          &(kernel->machine->mainMemory[physical_num * PageSize]),
          PageSize, virtual_num * PageSize); // write the portion of the file from main memory into swap space(virtual space)
        //cerr << "Remove page entry's physical page is: " << physical_num << "\n";
        //cerr << "Remove page entry's virtual page is: " << virtual_num << "\n";
        out_entry->physicalPage = -1; // set the least recent used one's physical page as -1 which means invalid
        out_entry->valid = FALSE; // set the least recent used one's valid as false
        page_Entry->physicalPage = physical_num; // set the physical page of the current swapping page as the out page's physical page number
        page_Entry->valid = TRUE; // set current swapping page as valid
        //cerr << "Insert page entry's physical page is: " << page_Entry->physicalPage << "\n";
        //cerr << "Insert page entry's virtual page is: " << page_Entry->virtualPage << "\n";
        kernel->swap_space->ReadAt(
          &(kernel->machine->mainMemory[physical_num * PageSize]),
          PageSize, page_Entry->virtualPage * PageSize); // read the portion of the file into main memory
        if (kernel->random_flag == 1) {
          kernel->random_list->set(page_Entry); // random data structure
        }
        else {
          kernel->LRU_Entries->set(page_Entry, page_Entry); // insert the page entry into the LRU data structure
        }
        //cerr << "-------------------------------------" << "\n";
      }
      return;
      ASSERTNOTREACHED();
      break;
    }
    default:
      cerr << "Unexpected user mode exception" << (int)which << "\n";
      break;
  }
  ASSERTNOTREACHED();
}

