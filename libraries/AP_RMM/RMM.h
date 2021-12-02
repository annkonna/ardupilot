#pragma once 

class RMM {
    void* pool;
    //int* pool_size;
    //bool* top_level;
    //int* free_offset;
    //int nested_count;
    //RMM* parent;

    RMM(void* parent_pool, int parent_free_offset, int size_in_bytes, RMM* p);
    void set_nested_count();

public:
    RMM(int size_in_bytes = 10 * 1024 * 1024); //default is 10 MB
    ~RMM();

    void* allocate(int size_in_bytes);
    void reset();

    RMM* create_nested_region(int size_in_bytes);

    int get_nested_count();

    int MemFree();

    int MemUsed();

    void print_status();

    void dump_region();

private:
    #define RMM_TOP_LEVEL       (0)
    #define RMM_SIZE            (0+sizeof(int))
    #define RMM_OFFSET          (0+sizeof(int)+sizeof(int))
    #define RMM_NESTED_COUNT    (RMM_OFFSET+sizeof(int))
    #define RMM_PARENT_PTR      (RMM_NESTED_COUNT+sizeof(int))
    #define RMM_FULL            (RMM_PARENT_PTR+sizeof(void*))

    void init_allocate(int size_in_bytes);
};
