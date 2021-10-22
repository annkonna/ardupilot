#pragma once 

class RMM {
     void* pool;
    int free_offset;
    int pool_size;

public:
    RMM(int size_in_MB = 10);
    ~RMM();

    void* allocate(int size_in_bytes);
    void reset();

    void print_status();
};
