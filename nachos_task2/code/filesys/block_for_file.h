#ifndef BLOCK_FOR_FILE_H
#define BLOCK_FOR_FILE_H

#include "disk.h"
#include "pbitmap.h"

#define MAX_BLOCKS_NUM (int) (SectorSize  / sizeof(int))

class Indirect {
public:
    Indirect(); // constructor
    int Allocate_block(PersistentBitmap *BM, int numSectors); //initialize indirect block
    void Deallocate_block(PersistentBitmap *BM); // deallocate file's data block
    void Fetch_From(int num); //initialize from disk
    void Write_Back(int num); // write back to disk
    int Byte_Covert_To_Sector(int num); // convert one byte offset to the disk sector
private:
    int data_sector[MAX_BLOCKS_NUM]; // data sector
};

class DoubleIndirect {
public:
    DoubleIndirect(); // constructor
    int Allocate_block(PersistentBitmap *BM, int numSectors); //initialize indirect block
    void Deallocate_block(PersistentBitmap *BM); // deallocate file's data block
    void Fetch_From(int num); //initialize from disk
    void Write_Back(int num); // write back to disk
    int Byte_Covert_To_Sector(int num); // convert one byte offset to the disk sector
private:
    int data_sector[MAX_BLOCKS_NUM]; // data sector
};

#endif    // BLOCK_FOR_FILE_H
