#include "block_for_file.h"
#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"
#include <string>

// constructor
Indirect::Indirect() {
    for(int i = 0; i < MAX_BLOCKS_NUM; i++) {
        data_sector[i] = -1;
    }
}

//initialize indirect block
int Indirect::Allocate_block(PersistentBitmap *BM, int numSectors) {
    if(numSectors < 0) {
        return -1;
    }
    if(BM->NumClear() < numSectors) {
        return -1;
    }
    int allocate_num = 0;
    for(int i = 0; i < MAX_BLOCKS_NUM && allocate_num < numSectors; i++) {
        if(data_sector[i] != -1) {
            continue;
        }
        data_sector[i] = BM->FindAndSet();
        allocate_num++;
    }
    return allocate_num;
}

// deallocate file's data block
void Indirect::Deallocate_block(PersistentBitmap *BM) {
    for(int i = 0; i < MAX_BLOCKS_NUM; i++) {
        int store = data_sector[i];
        if(store == -1) {
            continue;
        }
        BM->Clear(store);
    }
}

//initialize from disk
void Indirect::Fetch_From(int num) {
    kernel->synchDisk->ReadSector(num, (char*)this);
}

// write back to disk
void Indirect::Write_Back(int num) {
    kernel->synchDisk->WriteSector(num, (char*)this);
}

// convert one byte offset to the disk sector
int Indirect::Byte_Covert_To_Sector(int num) {
    int store = num/SectorSize;
    ASSERT(store < MAX_BLOCKS_NUM);
    int store1 = data_sector[store];
    ASSERT(store1 >= 0 && store1 < NumSectors);
    return store1;
}

// constructor
DoubleIndirect::DoubleIndirect() {
    for(int i = 0; i < MAX_BLOCKS_NUM; i++) {
        data_sector[i] = -1;
    }
}

//initialize indirect block
int DoubleIndirect::Allocate_block(PersistentBitmap *BM, int numSectors) {
    Indirect *block;
    if(numSectors < 0) {
        return -1;
    }
    if(BM->NumClear() < numSectors) {
        return -1;
    }
    int allocate_num = 0;
    for(int i = 0; i < MAX_BLOCKS_NUM && allocate_num < numSectors; i++) {
        block = new Indirect();
        if(data_sector[i] == -1) {
            data_sector[i] = BM->FindAndSet();
        }
        else {
            block->Fetch_From(data_sector[i]);
        }
        int result = block->Allocate_block(BM, numSectors - allocate_num);
        block->Write_Back(data_sector[i]);
        allocate_num += result;
        delete block;
    }
    return allocate_num;
}

// deallocate file's data block
void DoubleIndirect::Deallocate_block(PersistentBitmap *BM) {
    Indirect *block;
    for(int i = 0; i < MAX_BLOCKS_NUM; i++) {
        int sector = data_sector[i];
        if(sector == -1) {
            continue;
        }
        block = new Indirect();
        block->Fetch_From(sector);
        block->Deallocate_block(BM);
        BM->Clear(sector);
        delete block;
    }
}

//initialize from disk
void DoubleIndirect::Fetch_From(int num) {
  kernel->synchDisk->ReadSector(num, (char *)this);
}

// write back to disk
void DoubleIndirect::Write_Back(int num) {
  kernel->synchDisk->WriteSector(num, (char *)this);
}

// convert one byte offset to the disk sector
int DoubleIndirect::Byte_Covert_To_Sector(int num) {
    int store = num/SectorSize;
    Indirect *block = new Indirect();
    block->Fetch_From(data_sector[store/MAX_BLOCKS_NUM]);
    int store1 = block->Byte_Covert_To_Sector((store % MAX_BLOCKS_NUM) * SectorSize);
    delete block;
    ASSERT(store1 >= 0 && store1 < NumSectors);
    return store1;
}
