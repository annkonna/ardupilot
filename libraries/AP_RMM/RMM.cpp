#include <iostream>
#include "RMM.h"

using namespace std;

RMM::RMM(int size_in_bytes) {
    top_level = true;
    pool = new char[size_in_bytes];
    free_offset = 0;
    pool_size = size_in_bytes;
}

RMM::RMM(void* parent_pool, int parent_free_offset, int size_in_bytes) {
    top_level = false;
    pool = (char *)parent_pool + parent_free_offset;
    free_offset = 0;
    pool_size = size_in_bytes;
}

RMM::~RMM() {
    if (top_level) delete [] (char*)pool;
}

void* RMM::allocate(int size_in_bytes) {
    int temp_offset = free_offset;
    if (free_offset + size_in_bytes > pool_size)
        return nullptr;
    free_offset += size_in_bytes;
    return (char*)pool + temp_offset;
}

void RMM::reset() {
    free_offset = 0;
}

RMM* RMM::create_nested_region(int size_in_bytes) {
    if (free_offset + size_in_bytes > pool_size)
        return nullptr;

    RMM* pRet = new RMM(pool, free_offset, size_in_bytes);
    
    free_offset += size_in_bytes;
    return pRet;
}

void RMM::print_status() {
    cout << endl << "Region - Free bytes remaining in the pool: " << pool_size - free_offset << endl << endl;
}
