// kernel.h
//	Global variables for the Nachos kernel.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef KERNEL_H
#define KERNEL_H

#include "copyright.h"
#include "debug.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "alarm.h"
#include "filesys.h"
#include "machine.h"
#include "synch.h"
#include "filetable.h"
#include "bitmap.h"
#include "LRU.h"
#include "random.h"
#include <map>
#include <string>
class PostOfficeInput;
class PostOfficeOutput;
class SynchConsoleInput;
class SynchConsoleOutput;
class SynchDisk;

class Kernel {
  public:
    Kernel(int argc, char **argv);
    				// Interpret command line arguments
    ~Kernel();		        // deallocate the kernel
    
    void Initialize(); 		// initialize the kernel -- separated
				// from constructor because 
				// refers to "kernel" as a global
    void Initialize(char *quantum_num); 		// initialize with quantum
    void ThreadSelfTest();	// self test of threads and synchronization

    void ConsoleTest();         // interactive console self test

    void NetworkTest();         // interactive 2-machine network test
    
// These are public for notational convenience; really, 
// they're global variables used everywhere.

    Thread *currentThread;	// the thread holding the CPU
    Scheduler *scheduler;	// the ready list
    Interrupt *interrupt;	// interrupt status
    Statistics *stats;		// performance metrics
    Alarm *alarm;		// the software alarm clock    
    Machine *machine;           // the simulated CPU
    SynchConsoleInput *synchConsoleIn;
    SynchConsoleOutput *synchConsoleOut;
    SynchDisk *synchDisk;
    FileSystem *fileSystem;     
    PostOfficeInput *postOfficeIn;
    PostOfficeOutput *postOfficeOut;

    int hostName;               // machine identifier
    System_Wide_Table* File_Table;
    std::map<std::string, Semaphore*> *Read_Semaphore;
    std::map<std::string, Semaphore*> *Write_Semaphore;
    std::map<std::string, int> *Read_count;
    std::map<std::string, int> *Write_cout;
    std::map<std::string, bool> *removed_flag;
    //std::map<std::string, Lock*> *File_Lock;
    //std::map<std::string, Condition*> *File_Condition;
    Bitmap *thread_ids;
    OpenFile *swap_space;  // swap space
    int swap_space_counter;  // count the swap space
    LRU_Structure<TranslationEntry*> *LRU_Entries; // LRU data structure
    random_Structure<TranslationEntry*> *random_list; // random structure
    Bitmap *record_free_Map; // bit map structure which record the free positions in physical
    int random_flag;
  private:
    bool randomSlice;		// enable pseudo-random time slicing
    bool debugUserProg;         // single step user program
    double reliability;         // likelihood messages are dropped
    char *consoleIn;            // file to read console input from
    char *consoleOut;           // file to send console output to
#ifndef FILESYS_STUB
    bool formatFlag;          // format the disk if this is true
#endif
};


#endif // KERNEL_H


