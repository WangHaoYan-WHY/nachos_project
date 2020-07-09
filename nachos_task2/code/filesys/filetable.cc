#include "filetable.h"
#include "main.h"
#include <string>

// constructor
Table_OpenFile::Table_OpenFile(OpenFile *of, char *n, int num) {
  if (of == NULL) {
    std::cout << "open file is NULL" << std::endl;
  }
  else {
    std::cout << "open file is not NULL" << std::endl;
  }
  Open_File = of;
  count = 1;
  name = n;
  work_dir = num;
}
//deconstructor
Table_OpenFile::~Table_OpenFile() {
}
//get open file
OpenFile* Table_OpenFile::Get_openFile() {
  return Open_File;
}
// get count
int Table_OpenFile::Get_count() {
  return count;
}
// increase count
void Table_OpenFile::Inscrease_count() {
  count = count + 1;
}
// decrease count
void Table_OpenFile::Decrease_count() {
  count = count - 1;
}
// remove
void Table_OpenFile::remove() {
  kernel->fileSystem->Remove(name, work_dir);
}
// get name
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
  std::cout << "HHHHH" << std::endl;
  if (of == NULL) {
    std::cout << id << " ,,,,,,, " << std::endl;
    return;
  }
  openfiles_table[id] = new Table_OpenFile(of, n, num);
  std::string current_path(n);
  kernel->currentThread->space->name_id[current_path] = id;
  if (kernel->Read_Semaphore->find(current_path) == kernel->Read_Semaphore->end()) {
    (*kernel->Read_Semaphore)[current_path] = new Semaphore("read_s", 1);
  }
  if (kernel->Write_Semaphore->find(current_path) == kernel->Write_Semaphore->end()) {
    (*kernel->Write_Semaphore)[current_path] = new Semaphore("write_s", 1);
  }
  if (kernel->Read_count->find(current_path) == kernel->Read_count->end()) {
    (*kernel->Read_count)[current_path] = 0;
  }
  if (kernel->Write_cout->find(current_path) == kernel->Write_cout->end()) {
    (*kernel->Write_cout)[current_path] = 0;
  }
  /*
  if (kernel->File_Lock->find(current_path) == kernel->File_Lock->end()) {
    (*kernel->File_Lock)[current_path] = new Lock("lock");
  }
  if (kernel->File_Condition->find(current_path) == kernel->File_Condition->end()) {
    (*kernel->File_Condition)[current_path] = new Condition("condition");
  }
  */
  if (kernel->removed_flag->find(current_path) == kernel->removed_flag->end()) {
    (*kernel->removed_flag)[current_path] = FALSE;
  }
}
int ALL_OpenFiles_Table::GetLen() {
  return len;
}
void ALL_OpenFiles_Table::clear(OpenFileId id) {
  Table_OpenFile *t_of = openfiles_table[id];
  t_of->remove();
  openfiles_table[id] = NULL;
  //delete t_of;
}


System_Wide_Table::System_Wide_Table() {
  system_wide_table = new ALL_OpenFiles_Table();
  system_wide_table->Write(ConsoleInput1, NULL, "1", 1);
  system_wide_table->Write(ConsoleOutput1, NULL, "2", 1);
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
  std::string current_path(system_wide_table->Read_Table_Openfile(id)->GetName()); // get the current path
  if (kernel->removed_flag->find(current_path) != kernel->removed_flag->end()) {
    (*kernel->removed_flag)[current_path] = TRUE; // set removed flag as true
  }
  system_wide_table->Read_Table_Openfile(id)->Decrease_count();
  if (system_wide_table->Read_Table_Openfile(id)->Get_count() <= 0) {
    system_wide_table->clear(id); // actually remove the file from the open file table and delete from disk
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
void System_Wide_Table::Add_reference(OpenFileId id) {
  if (id < 0 || id >= system_wide_table->GetLen()) {
    return;
  }
  if (system_wide_table->Read_Table_Openfile(id) == NULL) {
    return;
  }
  system_wide_table->Read_Table_Openfile(id)->Inscrease_count();
}

Per_Process_OpenFile_Table::Per_Process_OpenFile_Table() {
  len = 100;
  open_file_id = new OpenFileId[len];
  open_file_id[0] = ConsoleInput1;
  open_file_id[1] = ConsoleOutput1;
  for (int i = 2; i < len; i++) {
    open_file_id[i] = -1;
  }
}

Per_Process_OpenFile_Table::Per_Process_OpenFile_Table(Per_Process_OpenFile_Table *parent_table) {
  len = 100;
  open_file_id = new OpenFileId[len];
  for (int i = 0; i < parent_table->len; i++) {
    open_file_id[i] = parent_table->open_file_id[i];
    kernel->File_Table->Add_reference(open_file_id[i]);
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
  for (int i = 2; i < len; i++) {
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
  kernel->File_Table->remove(open_file_id[id]); // remove the open file id from file table
  open_file_id[id] = -1; // delete this open file from the table

}
char* Per_Process_OpenFile_Table::GetName(OpenFileId id) {
  GetName(open_file_id[id]);
}