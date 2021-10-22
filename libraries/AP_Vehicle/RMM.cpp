#include <iostream>
#include "RMM.h"

using namespace std;

RMM::RMM(int size_in_MB) {
    pool = new char[size_in_MB * 1024 * 1024];
    free_offset = 0;
    pool_size = size_in_MB * 1024 * 1024;
}

RMM::~RMM() {
    delete [] (char*)pool;
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

void RMM::print_status() {
    cout << endl << "Region - Free bytes remaining in the pool: " << pool_size - free_offset << endl << endl;
}
