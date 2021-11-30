#include <iostream>
#include "RMM.h"

using namespace std;

RMM::RMM(int size_in_bytes) {
	pool = new char[size_in_bytes+FULL];
	bool* temp1 = (bool*) (pool+TOP_LEVEL);
	int* temp2 = (int*) (pool+SIZE);
	int* temp3 = (int*) (pool+OFFSET);
	*temp1 = true;
	*temp2 = size_in_bytes;
	*temp3 = 0+FULL;
}

RMM::RMM(void* parent_pool, int parent_free_offset, int size_in_bytes) {
	    //top_level = false;
	        pool = (char *)parent_pool + parent_free_offset+FULL;
		  //  free_offset = 0;
		bool* temp1 = (bool*) (pool+TOP_LEVEL);
		int* temp2 = (int*) (pool+SIZE);
		int* temp3 = (int*) (pool+OFFSET);
		*temp1 = false;
		*temp2 = size_in_bytes;
		*temp3 = 0+FULL;
		    //    pool_size = size_in_bytes;
}

RMM::~RMM() {
	bool* temp1 = (bool*) (pool+TOP_LEVEL);
	    if (*temp1 == true){
		    delete [] (char*)pool;
	    }
}

void* RMM::allocate(int size_in_bytes) {
	    int* freetemp = (int*) (pool+OFFSET);
	    int* sizetemp = (int*) (pool+SIZE);
	    int temp_offset = *freetemp;//free_offset;
	        if ((*freetemp) + size_in_bytes > (*sizetemp)+FULL)
			        return nullptr;
		    *freetemp += size_in_bytes;
		        return (char*)pool + temp_offset;
}

void RMM::reset() {
	    int* freetemp = (int*) (pool+OFFSET);
	    *freetemp = 0+FULL;
}

RMM* RMM::create_nested_region(int size_in_bytes) {
	int* freetemp = (int*) (pool+OFFSET);
	int* sizetemp = (int*) (pool+SIZE);
	    if (*freetemp + size_in_bytes+FULL > *sizetemp)
		            return nullptr;

	        RMM* pRet = new RMM(pool, *freetemp, size_in_bytes);

		    *freetemp += size_in_bytes;
		        return pRet;
}

void RMM::print_status() {
	int* freetemp = (int*) (pool+OFFSET);
	int* sizetemp = (int*) (pool+SIZE);

	    cout << endl << "Region - Free bytes remaining in the pool: " << (*sizetemp)+FULL - *freetemp << endl << endl;
}
