#ifndef FILETABLE_H
#define FILETABLE_H
#include "syscall.h"
#include "openfile.h"
#include <vector>
// table for open file
class Table_OpenFile {
public:
  Table_OpenFile(OpenFile *of, char *n, int num);
  ~Table_OpenFile();
  OpenFile *Get_openFile();
  int Get_count();
  void Inscrease_count();
  void Decrease_count();
  void remove();
  char* GetName();
private:
  OpenFile *Open_File;
  int count;
  char *name;
  int work_dir;
};

// table storing for all open files
class ALL_OpenFiles_Table {
public:
  ALL_OpenFiles_Table();
  ~ALL_OpenFiles_Table();
  Table_OpenFile *Read_Table_Openfile(OpenFileId id);
  void Write(OpenFileId id, OpenFile *of, char *n, int num);
  int GetLen();
  void clear(OpenFileId id);
private:
  Table_OpenFile **openfiles_table;
  int len;
};

// system wide open file table
class System_Wide_Table {
public:
  System_Wide_Table();
  ~System_Wide_Table();
  OpenFileId Insert(OpenFile *of, char *n, int num);
  OpenFile *convert(OpenFileId id);
  void remove(OpenFileId id);
  void Increase(OpenFileId id);
  char* GetName(OpenFileId id);
  void Add_reference(OpenFileId id);
private:
  ALL_OpenFiles_Table *system_wide_table;
};

// per process openfile table
class Per_Process_OpenFile_Table {
public:
  Per_Process_OpenFile_Table();
  Per_Process_OpenFile_Table(Per_Process_OpenFile_Table *parent_table);
  ~Per_Process_OpenFile_Table();
  OpenFileId Insert(OpenFile *of, char *n, int num);
  OpenFile *convert(OpenFileId id);
  void remove(OpenFileId id);
  OpenFileId *open_file_id;
  int len;
  char* GetName(OpenFileId id);
};

#endif // FILETABLE_H