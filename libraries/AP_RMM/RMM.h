#pragma once 
#include <AP_InertialSensor.h>

class RMM {
    void* pool;
    //int* pool_size;
    //bool* top_level;
    //int* free_offset;

    RMM(void* parent_pool, int parent_free_offset, int size_in_bytes);

public:
    RMM(int size_in_bytes = 10 * 1024 * 1024); //default is 10 MB
    ~RMM();

    void* allocate(int size_in_bytes);
    void reset();

    RMM* create_nested_region(int size_in_bytes);

    void print_status();

    void dump_region();

    //This Function Would Be Ardu Specific
    void repopulate_regions();

private:
    #define TOP_LEVEL 0
    #define SIZE 0+sizeof(int)
    #define OFFSET 0+sizeof(int)+sizeof(int)
    #define FULL 0+sizeof(int)+sizeof(int)+sizeof(int)

    void init_allocate(int size_in_bytes);
};
