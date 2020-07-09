// main.cc 
//	Driver code to initialize, selftest, and run the 
//	operating system kernel.  
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//              -s -x <nachos file> -ci <consoleIn> -co <consoleOut>
//              -f -cp <unix file> <nachos file>
//              -p <nachos file> -r <nachos file> -l -D
//              -n <network reliability> -m <machine id>
//              -z -K -C -N
//
//    -d causes certain debugging messages to be printed (see debug.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -ci specify file for console input (stdin is the default)
//    -co specify file for console output (stdout is the default)
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -K run a simple self test of kernel threads and synchronization
//    -C run an interactive console test
//    -N run a two-machine network test (see Kernel::NetworkTest)
//
//    Filesystem-related flags:
//    -f forces the Nachos disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//
//  Note: the file system flags are not used if the stub filesystem
//        is being used
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "main.h"
#include "filesys.h"
#include "openfile.h"
#include "sysdep.h"
#include <iostream>

// global variables
Kernel *kernel;
Debug *debug;

extern void ThreadTest(void);

//----------------------------------------------------------------------
// Cleanup
//	Delete kernel data structures; called when user hits "ctl-C".
//----------------------------------------------------------------------

static void 
Cleanup(int x) 
{     
    cerr << "\nCleaning up after signal " << x << "\n";
    delete kernel; 
}

//-------------------------------------------------------------------
// Constant used by "Copy" and "Print"
//   It is the number of bytes read from the Unix file (for Copy)
//   or the Nachos file (for Print) by each read operation
//-------------------------------------------------------------------
static const int TransferSize = 128;


#ifndef FILESYS_STUB
//----------------------------------------------------------------------
// Copy
//      Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

static void
Copy(char *from, char *to)
{
    int fd;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fd = OpenForReadWrite(from,FALSE)) < 0) {       
        printf("Copy: couldn't open input file %s\n", from);
        return;
    }
// Figure out length of UNIX file
    Lseek(fd, 0, 2);            
    fileLength = Tell(fd);
    Lseek(fd, 0, 0);
// Create a Nachos file of the same length
    DEBUG('f', "Copying file " << from << " of size " << fileLength <<  " to file " << to);
    if (!kernel->fileSystem->Create(to, fileLength, 1, 1)) {   // Create Nachos file
        printf("Copy: couldn't create output file %s\n", to);
        Close(fd);
        return;
    }
    openFile = kernel->fileSystem->Open(to, 1);
    ASSERT(openFile != NULL);
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead=ReadPartial(fd, buffer, sizeof(char)*TransferSize)) > 0)
        openFile->Write(buffer, amountRead);    
    delete [] buffer;
// Close the UNIX and the Nachos files
    delete openFile;
    Close(fd);
}

#endif // FILESYS_STUB

//----------------------------------------------------------------------
// Print
//      Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = kernel->fileSystem->Open(name, 1)) == NULL) {
        printf("Print: unable to open file %s\n", name);
        return;
    }

    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
        for (i = 0; i < amountRead; i++)
            printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;            // close the Nachos file
    return;
}

//----------------------------------------------------------------------
// RunUserProg
//      Run the user program in the given file.
//----------------------------------------------------------------------

void
RunUserProg(void *filename) {
    AddrSpace *space = new AddrSpace;
    ASSERT(space != (AddrSpace *)NULL);
    if (space->Load((char*)filename)) {  // load the program into the space
        space->Execute();         // run the program
    }
    ASSERTNOTREACHED();
}
// Run the user program in the given file
void
RunUP(void *filename) {
  if (kernel->currentThread->space->Load((char*)filename)) {  // load the user program into the space
    kernel->currentThread->space->Execute();      // run the user program
  }
  ASSERTNOTREACHED();
}

// implement running multiple user programs
void
RunMultipleUserProg(void *filename) {
  Thread *t = new Thread("newThread"); // new thread
  t->space = new AddrSpace(); // new address space
  ASSERT(t->space != (AddrSpace *)NULL);
  t->Fork((VoidFunctionPtr)RunUP, (void *)filename); // do thread's fork
}
//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Initialize kernel data structures
//	Call some test routines
//	Call "Run" to start an initial user program running
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------

int
main(int argc, char **argv)
{
  int uP_len = (argc - 1) / 2; // record the number of the userprograms
  char *userProgNames[uP_len]; // record the user programs' names
  uP_len = 0;
  int random_flag = -1;
  char *quantum_num = NULL; // store quantum
  int i;
  char *debugArg = "";
  char *userProgName = NULL;        // default is not to execute a user prog
  bool threadTestFlag = false;
  bool consoleTestFlag = false;
  bool networkTestFlag = false;
#ifndef FILESYS_STUB
  char *copyUnixFileName = NULL;    // UNIX file to be copied into Nachos
  char *copyNachosFileName = NULL;  // name of copied file in Nachos
  char *printFileName = NULL;
  char *removeFileName = NULL;
  bool dirListFlag = false;
  bool dumpFlag = false;
#endif //FILESYS_STUB
  // some command line arguments are handled here.
  // those that set kernel parameters are handled in
  // the Kernel constructor
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      ASSERT(i + 1 < argc);   // next argument is debug string
      debugArg = argv[i + 1];
      i++;
    }
    else if (strcmp(argv[i], "-z") == 0) {
      cout << copyright << "\n";
    }
    else if (strcmp(argv[i], "-x") == 0) { // run multiple user programs
      ASSERT(i + 1 < argc);
      userProgName = argv[i + 1];
      userProgNames[uP_len] = argv[i + 1]; // record the user programs' names
      uP_len++;
      i++;
    }
    else if (strcmp(argv[i], "-quantum") == 0) { // set quantum
      quantum_num = argv[i + 1];
    }
    else if (strcmp(argv[i], "-random") == 0) { // set random
      random_flag = 1;
    }
    else if (strcmp(argv[i], "-K") == 0) {
      threadTestFlag = TRUE;
    }
    else if (strcmp(argv[i], "-C") == 0) {
      consoleTestFlag = TRUE;
    }
    else if (strcmp(argv[i], "-N") == 0) {
      networkTestFlag = TRUE;
    }
#ifndef FILESYS_STUB
    else if (strcmp(argv[i], "-cp") == 0) {
      ASSERT(i + 2 < argc);
      copyUnixFileName = argv[i + 1];
      copyNachosFileName = argv[i + 2];
      i += 2;
    }
    else if (strcmp(argv[i], "-p") == 0) {
      ASSERT(i + 1 < argc);
      printFileName = argv[i + 1];
      i++;
    }
    else if (strcmp(argv[i], "-r") == 0) {
      ASSERT(i + 1 < argc);
      removeFileName = argv[i + 1];
      i++;
    }
    else if (strcmp(argv[i], "-l") == 0) {
      dirListFlag = true;
    }
    else if (strcmp(argv[i], "-D") == 0) {
      dumpFlag = true;
    }
#endif //FILESYS_STUB
    else if (strcmp(argv[i], "-u") == 0) {
      cout << "Partial usage: nachos [-z -d debugFlags]\n";
      cout << "Partial usage: nachos [-x programName]\n";
      cout << "Partial usage: nachos [-K] [-C] [-N]\n";
#ifndef FILESYS_STUB
      cout << "Partial usage: nachos [-cp UnixFile NachosFile]\n";
      cout << "Partial usage: nachos [-p fileName] [-r fileName]\n";
      cout << "Partial usage: nachos [-l] [-D]\n";
#endif //FILESYS_STUB
    }

  }
  debug = new Debug(debugArg);

  DEBUG(dbgThread, "Entering main");

  kernel = new Kernel(argc, argv);

  kernel->Initialize(quantum_num);

  CallOnUserAbort(Cleanup);		// if user hits ctl-C

  // at this point, the kernel is ready to do something
  // run some tests, if requested
  if (threadTestFlag) {
    //kernel->ThreadSelfTest();  // test threads and synchronization
    ThreadTest();
  }
  if (consoleTestFlag) {
    kernel->ConsoleTest();   // interactive test of the synchronized console
  }
  if (networkTestFlag) {
    kernel->NetworkTest();   // two-machine test of the network
  }

#ifndef FILESYS_STUB
  if (removeFileName != NULL) {
    kernel->fileSystem->Remove(removeFileName, 1);
  }
  if (copyUnixFileName != NULL && copyNachosFileName != NULL) {
    Copy(copyUnixFileName, copyNachosFileName);
  }
  if (dumpFlag) {
    kernel->fileSystem->Print();
  }
  if (dirListFlag) {
    kernel->fileSystem->List(1);
  }
  if (printFileName != NULL) {
    Print(printFileName);
  }
#endif // FILESYS_STUB
  if (random_flag == 1) {
    kernel->random_flag = 1;
  }
  // finally, run multiple user programs if requested to do so
  if (userProgName != NULL) {
    for (int m = 0; m < uP_len; m++) {
      RunMultipleUserProg(userProgNames[m]); // do run the user programs
    }
  }

  // NOTE: if the procedure "main" returns, then the program "nachos"
  // will exit (as any other normal program would).  But there may be
  // other threads on the ready list (started in SelfTest).  
  // We switch to those threads by saying that the "main" thread 
  // is finished, preventing it from returning.

  kernel->currentThread->Finish();

  ASSERTNOTREACHED();
}
