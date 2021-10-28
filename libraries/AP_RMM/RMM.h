#pragma once 

class RMM {
     void* pool;
    int free_offset;
    int pool_size;

public:
    RMM(int size_in_bytes = 10 * 1024 * 1024); //default is 10 MB
    ~RMM();

    void* allocate(int size_in_bytes);
    void reset();

    void print_status();
};
