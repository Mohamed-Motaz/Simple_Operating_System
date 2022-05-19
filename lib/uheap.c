#include <inc/lib.h>


void intializeUserHeap();
void* userHeapNextFitStrategy();
void* userHeapBestFitStrategy();
void* allocateInUserHeap();
uint32 getFreeSpaceForExpand();
void freeFromUserHeap();
uint32 userHeapIndex();
uint32 getReservedBlockSize();
void* getFreeSpaceForAllocationInUserHeap();

struct userHeapEntry{
	uint32 startAddress;    //same for all parts in a segment
	uint32 virtualAddress;  //real one, it doesn't change ever
	bool reserved;
};
int numOfUserHeapEntries = (USER_HEAP_MAX - USER_HEAP_START)/ PAGE_SIZE;
struct userHeapEntry userHeap[(USER_HEAP_MAX - USER_HEAP_START)/ PAGE_SIZE];
uint32 USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)USER_HEAP_START;
bool userHeapIntialized = 0;


void intializeUserHeap(){

	for (uint32 addr = USER_HEAP_START; addr < USER_HEAP_MAX; addr += PAGE_SIZE){
		int addrIndex = userHeapIndex(addr);
		userHeap[addrIndex].startAddress = 0;
		userHeap[addrIndex].virtualAddress = addr;
		userHeap[addrIndex].reserved = 0;
	}
}

uint32 userHeapIndex(uint32 addr){
	return (addr - USER_HEAP_START) / PAGE_SIZE;
}

void freeFromUserHeap(void* virtual_address,uint32 size){
	for(uint32 addr = (uint32)virtual_address; addr < (uint32)virtual_address + size; addr += PAGE_SIZE ){
		int addrIndex = userHeapIndex(addr);
		userHeap[addrIndex].reserved=0;
		userHeap[addrIndex].startAddress=0;
	}
}

uint32 getFreeSpaceForExpand(uint32 virtual_address){
	uint32 freeSize = 0;
	for(int addIndex = userHeapIndex(virtual_address); addIndex < numOfUserHeapEntries; addIndex++){
        if(userHeap[addIndex].startAddress == 0){
			freeSize+=PAGE_SIZE;
		}
		else
			break;
	}
	return freeSize;
}

uint32 getReservedBlockSize(uint32 startAddress){
	uint32 size = 0;

	for(uint32 addr = startAddress; addr < USER_HEAP_MAX; addr += PAGE_SIZE ){
		int addrIndex = userHeapIndex(addr);
		if(userHeap[addrIndex].startAddress == startAddress){
			size+=PAGE_SIZE;
		}
	}
	return size;
}

void* getFreeSpaceForAllocationInUserHeap(uint32 start, uint32 end, uint32 size){

	uint32 freeSize = 0;

	for (uint32 addr = start;addr < end; addr += PAGE_SIZE) {
		if (!userHeap[userHeapIndex(addr)].reserved) {
			//found a free page
			freeSize += PAGE_SIZE;
			if (freeSize == size) {
				addr -= size; //return ddr ptr properly
				addr += PAGE_SIZE; //since addr isnt incremented until the end of the iteration
				return (void*) addr;
			}
		} else
			//need to look for another segment
			freeSize = 0;
	}
	return NULL;
}

void* userHeapNextFitStrategy(uint32 size){

	void* freeSizePtr = getFreeSpaceForAllocationInUserHeap(USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR,USER_HEAP_MAX,size);

	freeSizePtr = (freeSizePtr == NULL)?
			getFreeSpaceForAllocationInUserHeap(USER_HEAP_START,USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR,size):freeSizePtr;

	USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (freeSizePtr != NULL)?
			(uint32)allocateInUserHeap((uint32)freeSizePtr, size):USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR;

	return freeSizePtr;
}
void* userHeapBestFitStrategy(uint32 size){

	uint32 freeSize = 0;
	uint32 minFreeSize = (USER_HEAP_MAX - USER_HEAP_START) + PAGE_SIZE; //just over the amount of userHeap
	uint32 startAddress = 0;

	for(uint32 addr = USER_HEAP_START; addr < USER_HEAP_MAX; addr+=PAGE_SIZE){

		if (!userHeap[userHeapIndex(addr)].reserved)
			//free page
			freeSize += PAGE_SIZE;

		else{
			if (freeSize >= size && freeSize < minFreeSize){
				minFreeSize = freeSize;
				startAddress = addr - minFreeSize;
			}
			freeSize = 0;
		}
	}

	//no space found

	if (!startAddress){
		//this condition is for when I reach the USER_HEAP_MAX without breaking
		if (freeSize >= size && freeSize < minFreeSize){
			minFreeSize = freeSize;
			startAddress = USER_HEAP_MAX - minFreeSize;
		}
		else
			return NULL;
	}
	allocateInUserHeap(startAddress, size);
	return (void*)startAddress;
}

void* allocateInUserHeap(uint32 startAddress, uint32 size){

	int addrIndex = userHeapIndex(startAddress);
	while(size > 0){
		userHeap[addrIndex].reserved = 1;
		userHeap[addrIndex].startAddress = startAddress;
		addrIndex++;
		size-=PAGE_SIZE;
	}
	return (void*)userHeap[(addrIndex % numOfUserHeapEntries)].virtualAddress;
}


void* malloc(uint32 size)
{
	//TODO:DONE [PROJECT 2022 - [9] User Heap malloc()] [User Side]

	if(!userHeapIntialized){
		intializeUserHeap();
		userHeapIntialized = 1;
	}
	size = ROUNDUP(size, PAGE_SIZE);

	void* address = (sys_isUHeapPlacementStrategyNEXTFIT())?userHeapNextFitStrategy(size):userHeapBestFitStrategy(size);
	if(address != NULL){
		sys_allocateMem((uint32)address,size);
	}
	return address;
}

void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	panic("smalloc() is not required ..!!");
	return NULL;
}

void* sget(int32 ownerEnvID, char *sharedVarName)
{
	panic("sget() is not required ..!!");
	return 0;
}

void free(void* virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [11] User Heap free()] [User Side]

	if(!userHeapIntialized){
		intializeUserHeap();
		userHeapIntialized = 1;
	}

	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
	uint32 size = getReservedBlockSize((uint32)virtual_address);
	freeFromUserHeap(virtual_address,size);
	sys_freeMem((uint32)virtual_address,size);

}


void sfree(void* virtual_address)
{
	panic("sfree() is not requried ..!!");
}

void* realloc(void *virtual_address, uint32 new_size)
{
	//TODO: DONE [PROJECT 2022 - BONUS3] User Heap Realloc [User Side]

	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);
	new_size = ROUNDUP(new_size, PAGE_SIZE);

	if(virtual_address == NULL)
		return malloc(new_size);
	if(!new_size){
		free(virtual_address);
		return NULL;
	}

	uint32 oldSize = getReservedBlockSize((uint32)virtual_address);
	uint32 freeSize = getFreeSpaceForExpand((uint32)virtual_address+oldSize);

	if(new_size <= oldSize){
		freeFromUserHeap(virtual_address+new_size,oldSize - new_size);
		USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)(virtual_address+(new_size));
		return virtual_address;
	}
	else if(new_size <= (oldSize + freeSize)){
		USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)allocateInUserHeap((uint32)virtual_address,new_size);
		sys_allocateMem((uint32)virtual_address+oldSize,new_size-oldSize);
		return virtual_address;
	}
	else {

		void* newAddress = (sys_isUHeapPlacementStrategyNEXTFIT())?userHeapNextFitStrategy(new_size):userHeapBestFitStrategy(new_size);
		if(newAddress != NULL){
			if(new_size <= oldSize){
				sys_moveMem((uint32)virtual_address,(uint32)newAddress,new_size);
			}
			else{
				sys_moveMem((uint32)virtual_address,(uint32)newAddress,oldSize);
				sys_allocateMem(((uint32)newAddress+oldSize),new_size-oldSize);
			}
			free(virtual_address);
			return newAddress;
		}
	}
	return NULL;
}
