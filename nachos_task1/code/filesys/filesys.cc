// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "main.h"
#include <string>

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

OpenFile* freeMapFile; // free disk blocks
OpenFile* directoryFile; // directory

int parse_path(char **path, int sector_num) {
    std::string current_path(*path), directory_name;
    std::string::size_type position;
    while((position = current_path.find("/")) != std::string::npos) {
        directory_name = current_path.substr(0, position);
        current_path = current_path.substr(position + 1, current_path.size());
        Directory *dir = new Directory(NumDirEntries);
        OpenFile *dir_File = new OpenFile(sector_num);
        dir->FetchFrom(dir_File);
        if(!dir->judge_directory((char *)directory_name.c_str())) {
            return -1;
        }
        sector_num = dir->Find((char*)directory_name.c_str());
        delete dir; delete dir_File;
    }
    char *file_name = new char[current_path.size() + 1];
    std::copy(current_path.begin(), current_path.end(), file_name);
    file_name[current_path.size()] = '\0';
    *path = file_name;
    return sector_num;
}

//----------------------------------------------------------------------
// FileSystem::FileSystem
//     Initialize the file system.  If format = TRUE, the disk has
//    nothing on it, and we need to initialize the disk to contain
//    an empty directory, and a bitmap of free sectors (with almost but
//    not all of the sectors marked as free).
//
//    If format = FALSE, we just have to open the files
//    representing the bitmap and the directory.
//
//    "format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{
    DEBUG(dbgFile, "Initializing the file system.");
    if (format) {
        PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;
        
        DEBUG(dbgFile, "Formatting the file system.");
        
        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);
        freeMap->Mark(DirectorySector);
        
        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!
        
        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));
        
        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).
        
        DEBUG(dbgFile, "Writing headers back to disk.");
        mapHdr->WriteBack(FreeMapSector);
        dirHdr->WriteBack(DirectorySector);
        
        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.
        
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
        
        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.
        
        DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
        freeMap->WriteBack(freeMapFile);     // flush changes to disk
        directory->WriteBack(directoryFile);
        if (debug->IsEnabled('f')) {
            freeMap->Print();
            directory->Print();
        }
        delete freeMap;
        delete directory;
        delete mapHdr;
        delete dirHdr;
    } else {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
}

//----------------------------------------------------------------------
// FileSystem::Create
//     Create a file in the Nachos file system (similar to UNIX create).
//    Since we can't increase the size of files dynamically, we have
//    to give Create the initial size of the file.
//
//    The steps to create a file are:
//      Make sure the file doesn't already exist
//        Allocate a sector for the file header
//       Allocate space on disk for the data blocks for the file
//      Add the name to the directory
//      Store the new file header on disk
//      Flush the changes to the bitmap and the directory back to disk
//
//    Return TRUE if everything goes ok, otherwise, return FALSE.
//
//     Create fails if:
//           file is already in directory
//         no free space for file header
//         no free entry for file in directory
//         no free space for data blocks for the file
//
//     Note that this implementation assumes there is no concurrent access
//    to the file system!
//
//    "name" -- name of file to be created
//    "initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool
FileSystem::Create(char *name, int initialSize, int protection, int work_dir_sector)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;
    
    DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);
    int work_sector = parse_path(&name, work_dir_sector);
    if(work_sector < 0) {
        printf("The path: %s is not legal", name);
        return FALSE;
    }
    directory = new Directory(NumDirEntries);
    OpenFile *dir_File = new OpenFile(work_sector);
    directory->FetchFrom(dir_File);
    if (directory->Find(name) != -1)
        success = FALSE;            // file is already in directory
    else {
        freeMap = new PersistentBitmap(freeMapFile,NumSectors);
        sector = freeMap->FindAndSet();    // find a sector to hold the file header
        
        if (sector == -1)
            success = FALSE;        // no free block for file header
        else if (!directory->Add(name, sector))
            success = FALSE;    // no space in directory
        else {
            hdr = new FileHeader;
            if (!hdr->Allocate(freeMap, initialSize))
                success = FALSE;    // no space on disk for data
            else {
                success = TRUE;
                // everthing worked, flush all changes back to disk
                hdr->set_protection(protection);
                hdr->WriteBack(sector);
                directory->WriteBack(dir_File);
                freeMap->WriteBack(freeMapFile);
            }
            delete hdr;
        }
        delete freeMap;
    }
    delete directory; delete dir_File;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
//     Open a file for reading and writing.
//    To open a file:
//      Find the location of the file's header, using the directory
//      Bring the header into memory
//
//    "name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFileId
FileSystem::new_Open(char *name, int mode, int work_dir_sector)
{
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;
    DEBUG(dbgFile, "Opening file" << name);

    int work_sector = parse_path(&name, work_dir_sector);
    if(work_sector < 0) {
        printf("The path: %s is not legal", name);
        return 0;
    }
    FileHeader *fileHdr;
    OpenFile *dir_File = new OpenFile(work_sector);
    directory->FetchFrom(dir_File);
    sector = directory->Find(name);
    if (sector >= 0) {
      fileHdr = new FileHeader;
      fileHdr->FetchFrom(sector);
      if ((fileHdr->get_protection() & mode) != mode) {
        printf("The protection permission is not permited");
        return 0;
      }
      openFile = new OpenFile(sector);    // name was found in directory
    }
    OpenFileId res = kernel->File_Table->Insert(openFile, name, 1);
    delete directory; delete dir_File;
    return res;                // return NULL if not found
}

OpenFile*
FileSystem::Open(char *name, int mode)
{
  Directory *directory = new Directory(NumDirEntries);
  OpenFile *openFile = NULL;
  int sector;
  DEBUG(dbgFile, "Opening file" << name);

  int work_sector = parse_path(&name, 1);
  if (work_sector < 0) {
    printf("The path: %s is not legal", name);
    return NULL;
  }
  FileHeader *fileHdr;
  OpenFile *dir_File = new OpenFile(work_sector);
  directory->FetchFrom(dir_File);
  sector = directory->Find(name);
  if (sector >= 0) {
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);
    if ((fileHdr->get_protection() & mode) != mode) {
      printf("The protection permission is not permited");
      return NULL;
    }
    openFile = new OpenFile(sector);    // name was found in directory
  }
  OpenFileId res = kernel->File_Table->Insert(openFile, name, 1);
  delete directory; delete dir_File;
  return openFile;                // return NULL if not found
}

// remove a file
bool
FileSystem::Remove_File(char *name, int work_dir_sector)
{
  if (kernel->name_id.find(name) == kernel->name_id.end()) {
    return FALSE;
  }
  kernel->File_Table->remove(kernel->name_id[name]);
  return TRUE;
}
//----------------------------------------------------------------------
// FileSystem::Remove
//     Delete a file from the file system.  This requires:
//        Remove it from the directory
//        Delete the space for its header
//        Delete the space for its data blocks
//        Write changes to directory, bitmap back to disk
//
//    Return TRUE if the file was deleted, FALSE if the file wasn't
//    in the file system.
//
//    "name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool
FileSystem::Remove(char *name, int work_dir_sector)
{
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;
    int work_sector = parse_path(&name, work_dir_sector);
    if(work_sector < 0) {
        printf("The path: %s is not legal", name);
        return FALSE;
    }
    OpenFile *dir_File = new OpenFile(work_sector);
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(dir_File);
    sector = directory->Find(name);
    if (sector == -1 || directory->judge_directory(name)) {
        delete directory;
        delete dir_File;
        return FALSE;             // file not found
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);
    freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    fileHdr->Deallocate(freeMap);          // remove data blocks
    freeMap->Clear(sector);            // remove header block
    directory->Remove(name);
    
    freeMap->WriteBack(freeMapFile);        // flush to disk
    directory->WriteBack(dir_File);        // flush to disk
    delete fileHdr;
    delete directory;
    delete freeMap;
    delete dir_File;
    return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
//     List all the files in the file system directory.
//----------------------------------------------------------------------

void
FileSystem::List(int Dir_Sector)
{
    OpenFile *dir_File = new OpenFile(Dir_Sector);
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(dir_File);
    directory->List(0);
    delete dir_File;
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
//     Print everything about the file system:
//      the contents of the bitmap
//      the contents of the directory
//      for each file in the directory,
//          the contents of the file header
//          the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    Directory *directory = new Directory(NumDirEntries);
    
    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();
    
    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();
    
    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();
    
    directory->FetchFrom(directoryFile);
    directory->Print();
    
    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
}

OpenFile *Get_Free_Map_File() {
    return freeMapFile;
}

OpenFile *Get_Directory_File() {
    return directoryFile;
}

bool FileSystem::Make_Dir(char *name, int initialSize, int work_dir_sector) {
  Directory *directory;
  PersistentBitmap *freeMap;
  FileHeader *hdr;
  int sector;
  bool success;
  int work_sector = parse_path(&name, work_dir_sector);
  if (work_sector < 0) {
    printf("The path: %s is not legal", name);
    return FALSE;
  }
  directory = new Directory(NumDirEntries);
  OpenFile *dir_File = new OpenFile(work_sector);
  directory->FetchFrom(dir_File);
  if (directory->Find(name) != -1)
    success = FALSE;            // file is already in directory
  else {
    freeMap = new PersistentBitmap(freeMapFile, NumSectors);
    sector = freeMap->FindAndSet();    // find a sector to hold the file header

    if (sector == -1)
      success = FALSE;        // no free block for file header
    else if (!directory->Add_to_Directory(name, sector))
      success = FALSE;    // no space in directory
    else {
      hdr = new FileHeader;
      if (!hdr->Allocate(freeMap, initialSize))
        success = FALSE;    // no space on disk for data
      else {
        success = TRUE;
        // everthing worked, flush all changes back to disk
        hdr->WriteBack(sector);
        directory->WriteBack(dir_File);
        freeMap->WriteBack(freeMapFile);
        Directory *new_Dir = new Directory(NumDirEntries);
        OpenFile *new_File = new OpenFile(sector);
        ASSERT(new_Dir->Add_to_Directory(".", sector));
        ASSERT(new_Dir->Add_to_Directory("..", work_sector));
        new_Dir->WriteBack(new_File);
        delete new_Dir; delete new_File;
      }
      delete hdr;
    }
    delete freeMap;
  }
  delete directory; delete dir_File;
  return success;
}

bool FileSystem::Change_Dir(char *name, int initialSize, int work_dir_sector) {
  Directory *directory;
  int sector;
  int work_sector = parse_path(&name, work_dir_sector);
  if (work_sector < 0) {
    printf("The path: %s is not legal", name);
    return -1;
  }
  OpenFile *dir_File = new OpenFile(work_sector);
  directory = new Directory(NumDirEntries);
  directory->FetchFrom(dir_File);
  if (!directory->judge_directory(name)) {
    printf("The path: %s is not found", name);
    delete directory; delete dir_File;
    return -1;
  }
  sector = directory->Find(name);
  delete directory; delete dir_File;
  return sector;
}
#endif // FILESYS_STUB
