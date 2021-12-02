#include <iostream>
#include "RMM.h"

using namespace std;

RMM::RMM(int size_in_bytes) {
//	cout << "Size To Allocate " << size_in_bytes << endl;
	//top_level = true;
	pool = new char[size_in_bytes+RMM_FULL];
	// nested_count = 1;
	//	    free_offset = 0;
	//	        pool_size = size_in_byt
	bool* temp1 = (bool*) ((char*)pool+RMM_TOP_LEVEL);
	int* temp2 = (int*) ((char*)pool+RMM_SIZE);
	int* temp3 = (int*) ((char*)pool+RMM_OFFSET);
	int* temp4 = (int*) ((char*)pool+RMM_NESTED_COUNT);
	RMM** temp5 = (RMM**) ((char*)pool+RMM_PARENT_PTR);

//	cout << pool << " "<< temp1<<" "<<temp2<< " "<< temp3<< endl;
	//int* temp2 = (int*) (pool+SIZE);
	//int* temp3 = (int*) (pool+OFFSET);
	*temp1 = true;
	*temp2 = size_in_bytes+RMM_FULL;
	*temp3 = 0+RMM_FULL;
	*temp4 = 1; 	
	*temp5 = nullptr;
}

RMM::RMM(void* parent_pool, int parent_free_offset, int size_in_bytes, RMM *p) {
	//top_level = false;
	pool = (char *)parent_pool + parent_free_offset+RMM_FULL;
	*(RMM **)pool = nullptr;
	//  free_offset = 0;
	bool* temp1 = (bool*) ((char*)pool+RMM_TOP_LEVEL);
	int* temp2 = (int*) ((char*)pool+RMM_SIZE);
	int* temp3 = (int*) ((char*)pool+RMM_OFFSET);
	int* temp4 = (int*) ((char*)pool+RMM_NESTED_COUNT);
	RMM** temp5 = (RMM**) ((char*)pool+RMM_PARENT_PTR);
	*temp1 = false;
	*temp2 = size_in_bytes+RMM_FULL;
	*temp3 = 0+RMM_FULL;
	*temp4 = 0;
	*temp5 = p;
	//    pool_size = size_in_bytes;
	set_nested_count();
}

RMM::~RMM() {
	RMM** parent = (RMM **)((char*)pool + RMM_PARENT_PTR);
	if (*parent == nullptr) delete [] (char*)pool;
}

void* RMM::allocate(int size_in_bytes) {
	int* freetemp = (int*) ((char*)pool+RMM_OFFSET);
	int* sizetemp = (int*) ((char*)pool+RMM_SIZE);
	int temp_offset = *freetemp;//free_offset;
	if ((*freetemp) + size_in_bytes > (int)((*sizetemp)+RMM_FULL))
		return nullptr;
	*freetemp += size_in_bytes;
	return (char*)pool + temp_offset;
}

void RMM::reset() {
	int* freetemp = (int*) ((char*)pool+RMM_OFFSET);
	*freetemp = 0+RMM_FULL;
}

RMM* RMM::create_nested_region(int size_in_bytes) {
	int* freetemp = (int*) ((char*)pool+RMM_OFFSET);
	int* sizetemp = (int*) ((char*)pool+RMM_SIZE);
	if ((int)(*freetemp + size_in_bytes+RMM_FULL) > *sizetemp)
		return nullptr;
	RMM* pRet = new RMM(pool, *freetemp, size_in_bytes, this);
		    
	*freetemp += size_in_bytes;
	return pRet;
}

void RMM::set_nested_count() {
	RMM** parent = (RMM **)((char*)pool + RMM_PARENT_PTR);
    if (*parent)
        (*parent)->set_nested_count();
    else
		{ 
			int* p_nested_count = (int *)((char*)pool+RMM_NESTED_COUNT);
			*p_nested_count = *p_nested_count + 1;
		}

}

int RMM::get_nested_count() {
    return *((char*)pool+RMM_NESTED_COUNT);
}

int RMM::MemFree() {
	int* p_pool_size = (int*) ((char*)pool+RMM_SIZE);
	int* p_free_offset = (int*) ((char*)pool+RMM_OFFSET);
    int free_bytes = *p_pool_size - *p_free_offset;
    return free_bytes;
}

int RMM::MemUsed() {
	int* p_free_offset = (int*) ((char*)pool+RMM_OFFSET);
    int memory_used = sizeof(RMM) * get_nested_count() + *p_free_offset;
    return memory_used;
}

void RMM::print_status() {
	cout << endl << "Region - Free bytes remaining in the pool: " << MemFree() << endl << endl;
    cout << endl << "Region - Memory used by the pool and region objects: " << MemUsed() << endl << endl;
}
