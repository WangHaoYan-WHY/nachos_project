/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__ 
#define __USERPROG_KSYSCALL_H__ 

#include "kernel.h"




void SysHalt()
{
  kernel->interrupt->Halt();
}


int SysAdd(int op1, int op2)
{
  return op1 + op2;
}

// implement console read
int SysRead(int addr, int num)
{
  printf("-------------------------------------\n");
  printf("Do console read operation\n");
  printf("The read content is:\n");
  char *sequenceRead = "abc";
  int addr1 = addr;
  for (int i = 0; i < num; i++) {
    int store = (int)sequenceRead[i];
    kernel->machine->WriteMem(addr++, 1, store); // write memo
  }
  for (int i = 0; i < num; i++) {
    int *result = new int(0);
    kernel->machine->ReadMem(addr1++, 1, result); //read memo
    printf("%c", ((char)*result));
  }
  printf("\n-------------------------------------\n");
  return num;
}

// implement console write
int SysWrite(int addr, int num)
{
  printf("-------------------------------------\n");
  printf("Do console write operation:\n");
  printf("Write content into console:\n");
  int *temp = new int(0);
  for (int i = 0; i < num; i++) {
    kernel->machine->ReadMem(addr++, 1, temp); // read memo
    printf("%c", (char)*temp);
  }
  printf("\n-------------------------------------\n");
  return num;
}




#endif /* ! __USERPROG_KSYSCALL_H__ */
