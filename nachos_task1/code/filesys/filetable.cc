#include "filetable.h"
#include "main.h"
#include <string>


Table_OpenFile::Table_OpenFile(OpenFile *of, char *n, int num) {
  Open_File = of;
  count = 1;
  name = n;
  work_dir = num;
}
Table_OpenFile::~Table_OpenFile() {
  delete Open_File;
}
OpenFile* Table_OpenFile::Get_openFile() {
  return Open_File;
}
int Table_OpenFile::Get_count() {
  return count;
}
void Table_OpenFile::Inscrease_count() {
  count = count + 1;
}
void Table_OpenFile::Decrease_count() {
  count = count - 1;
}
void Table_OpenFile::remove() {
  kernel->file_system->Remove(name, work_dir);
}
char* Table_OpenFile::GetName() {
  return name;
}


ALL_OpenFiles_Table::ALL_OpenFiles_Table() {
  len = 100;
  openfiles_table = new Table_OpenFile*[len];
}
ALL_OpenFiles_Table::~ALL_OpenFiles_Table() {
  delete[] openfiles_table;
}
Table_OpenFile* ALL_OpenFiles_Table::Read_Table_Openfile(OpenFileId id) {
  return openfiles_table[id];
}
void ALL_OpenFiles_Table::Write(OpenFileId id, OpenFile *of, char *n, int num) {
  if (of == NULL) {
    return;
  }
  openfiles_table[id] = new Table_OpenFile(of, n, num);
  std::string current_path(n);
  kernel->name_id[current_path] = id;
  if (kernel->Read_Semaphore->find(n) == kernel->Read_Semaphore->end()) {
    (*kernel->Read_Semaphore)[n] = new Semaphore("read_s", 1);
  }
  if (kernel->Write_Semaphore->find(n) == kernel->Write_Semaphore->end()) {
    (*kernel->Write_Semaphore)[n] = new Semaphore("write_s", 1);
  }
  if (kernel->Read_count->find(n) == kernel->Read_count->end()) {
    (*kernel->Read_count)[n] = 0;
  }
  if (kernel->Write_cout->find(n) == kernel->Write_cout->end()) {
    (*kernel->Write_cout)[n] = 0;
  }
  if (kernel->File_Lock->find(n) == kernel->File_Lock->end()) {
    (*kernel->File_Lock)[n] = new Lock("lock");
  }
  if (kernel->File_Condition->find(n) == kernel->File_Condition->end()) {
    (*kernel->File_Condition)[n] = new Condition("condition");
  }
}
int ALL_OpenFiles_Table::GetLen() {
  return len;
}
void ALL_OpenFiles_Table::clear(OpenFileId id) {
  Table_OpenFile *t_of = openfiles_table[id];
  t_of->remove();
  openfiles_table[id] = NULL;
  delete t_of;
}


System_Wide_Table::System_Wide_Table() {
  system_wide_table = new ALL_OpenFiles_Table();
  system_wide_table->Write(0, NULL, "1", 1);
  system_wide_table->Write(1, NULL, "2", 1);
}
System_Wide_Table::~System_Wide_Table() {
  delete system_wide_table;
}
OpenFileId System_Wide_Table::Insert(OpenFile *of, char *n, int num) {
  for (int i = 0; i < system_wide_table->GetLen(); i++) {
    if (system_wide_table->Read_Table_Openfile(i) == NULL) {
      system_wide_table->Write(i, of, n, num);
      return i;
    }
  }
  return -1;
}
OpenFile* System_Wide_Table::convert(OpenFileId id) {
  if (id < 0 || id >= system_wide_table->GetLen()) {
    return NULL;
  }
  if (system_wide_table->Read_Table_Openfile(id) == NULL) {
    return NULL;
  }
  return system_wide_table->Read_Table_Openfile(id)->Get_openFile();
}
void System_Wide_Table::remove(OpenFileId id) {
  if (id < 0 || id >= system_wide_table->GetLen()) {
    return;
  }
  if (system_wide_table->Read_Table_Openfile(id) == NULL) {
    return;
  }
  system_wide_table->Read_Table_Openfile(id)->Decrease_count();
  if (system_wide_table->Read_Table_Openfile(id)->Get_count() <= 0) {
    system_wide_table->clear(id);
  }
  return;
}
void System_Wide_Table::Increase(OpenFileId id) {
  if (id < 0 || id >= system_wide_table->GetLen()) {
    return;
  }
  if (system_wide_table->Read_Table_Openfile(id) == NULL) {
    return;
  }
  system_wide_table->Read_Table_Openfile(id)->Inscrease_count();
}
char* System_Wide_Table::GetName(OpenFileId id) {
  if (id < 0 || id >= system_wide_table->GetLen()) {
    return NULL;
  }
  system_wide_table->Read_Table_Openfile(id)->GetName();
}


Per_Process_OpenFile_Table::Per_Process_OpenFile_Table() {
  len = 100;
  open_file_id = new OpenFileId[len];
  for (int i = 0; i < len; i++) {
    open_file_id[i] = -1;
  }
}
Per_Process_OpenFile_Table::~Per_Process_OpenFile_Table() {
  for (int i = 0; i < len; i++) {
    kernel->File_Table->remove(open_file_id[i]);
  }
  delete[] open_file_id;
}
OpenFileId Per_Process_OpenFile_Table::Insert(OpenFile *of, char *n, int num) {
  OpenFileId store = kernel->File_Table->Insert(of, n, num);
  OpenFileId local_id = -1;
  for (int i = 0; i < len; i++) {
    if (open_file_id[i] == -1) {
      open_file_id[i] = store;
      local_id = i;
      break;
    }
  }
  return local_id;
}
OpenFile* Per_Process_OpenFile_Table::convert(OpenFileId id) {
  if (id < 0 || id >= len) {
    return NULL;
  }
  return kernel->File_Table->convert(open_file_id[id]);
}
void Per_Process_OpenFile_Table::remove(OpenFileId id) {
  if (id < 0 || id >= len) {
    return;
  }
  kernel->File_Table->remove(open_file_id[id]);
  open_file_id[id] = -1;
}