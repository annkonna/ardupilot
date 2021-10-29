#pragma once 

class RMM {
     void* pool;
    int free_offset;
    int pool_size;
    bool top_level;

    RMM(void* parent_pool, int parent_free_offset, int size_in_bytes);

public:
    RMM(int size_in_bytes = 10 * 1024 * 1024); //default is 10 MB
    ~RMM();

    void* allocate(int size_in_bytes);
    void reset();

    RMM* create_nested_region(int size_in_bytes);

    void print_status();
};
