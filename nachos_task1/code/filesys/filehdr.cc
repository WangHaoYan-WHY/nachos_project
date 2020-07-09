// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

FileHeader::FileHeader() {
    for(int i = 0; i < NumDirect; i++) {
        dataSectors[i] = -1;
    }
    numBytes = 0; numSectors = 0;
}
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize)
{
    int num_Sectors = divRoundUp(fileSize, SectorSize);
    if(freeMap->NumClear() < num_Sectors) {
        return FALSE;
    }
    DoubleIndirect *d_block;
    std::cout << "using double indirect initialize the file header" << std::endl;
    int allocated_num = 0;
    for(int i = 0; i < NumDirect && allocated_num < num_Sectors; i++) {
        d_block = new DoubleIndirect();
        if(dataSectors[i] == -1) {
          dataSectors[i] = freeMap->FindAndSet();
        }
        else {
            d_block->Fetch_From(dataSectors[i]);
        }
        int res = d_block->Allocate_block(freeMap, num_Sectors - allocated_num);
        d_block->Write_Back(dataSectors[i]);
        allocated_num += res;
        delete d_block;
    }
    numBytes += fileSize;
    num_Sectors += divRoundUp(fileSize, SectorSize);
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(PersistentBitmap *freeMap)
{
    DoubleIndirect *d_block;
    for(int i = 0; i < NumDirect; i++) {
        int sector = dataSectors[i];
        if(sector == -1) {
            continue;
        }
        d_block = new DoubleIndirect();
        d_block->Fetch_From(sector);
        d_block->Deallocate_block(freeMap);
        freeMap->Clear(sector);
        delete d_block;
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    kernel->synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    kernel->synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    int store = offset/SectorSize;
    DoubleIndirect *d_block = new DoubleIndirect();
    d_block->Fetch_From(dataSectors[store/(MAX_BLOCKS_NUM * MAX_BLOCKS_NUM)]);
    int store1 = d_block->Byte_Covert_To_Sector(offset);
    delete d_block;
    return store1;
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
	printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	kernel->synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		printf("%c", data[j]);
            else
		printf("\\%x", (unsigned char)data[j]);
	}
        printf("\n"); 
    }
    delete [] data;
}

void FileHeader::set_protection(int p) {
    protection = p;
}

int FileHeader::get_protection() {
  return protection;
}