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

	for (uint32 curAddr = USER_HEAP_START; curAddr < USER_HEAP_MAX; curAddr += PAGE_SIZE){
		int curAddrIndex = userHeapIndex(curAddr);
		userHeap[curAddrIndex].startAddress = 0;
		userHeap[curAddrIndex].virtualAddress = curAddr;
		userHeap[curAddrIndex].reserved = 0;
	}
}

uint32 userHeapIndex(uint32 curAddr){
	return (curAddr - USER_HEAP_START) / PAGE_SIZE;
}

void freeFromUserHeap(void* virtual_address,uint32 size){
	for(uint32 curAddr = (uint32)virtual_address; curAddr < (uint32)virtual_address + size; curAddr += PAGE_SIZE ){
		int curAddrIndex = userHeapIndex(curAddr);
		userHeap[curAddrIndex].reserved=0;
		userHeap[curAddrIndex].startAddress=0;
	}
}

uint32 getFreeSpaceForExpand(uint32 virtual_address){
	uint32 freeSize = 0;
	for(int curAddrIndex = userHeapIndex(virtual_address); curAddrIndex < numOfUserHeapEntries; curAddrIndex++){
        if(userHeap[curAddrIndex].startAddress == 0){
			freeSize+=PAGE_SIZE;
		}
		else
			break;
	}
	return freeSize;
}

uint32 getReservedBlockSize(uint32 startAddress){
	uint32 size = 0;

	for(uint32 curAddr = startAddress; curAddr < USER_HEAP_MAX; curAddr += PAGE_SIZE ){
		int curAddrIndex = userHeapIndex(curAddr);
		if(userHeap[curAddrIndex].startAddress == startAddress){
			size+=PAGE_SIZE;
		}
	}
	return size;
}

void* getFreeSpaceForAllocationInUserHeap(uint32 start, uint32 end, uint32 size){

	uint32 freeSize = 0;

	for (uint32 curAddr = start;curAddr < end; curAddr += PAGE_SIZE) {
		if (!userHeap[userHeapIndex(curAddr)].reserved) {
			//found a free page
			freeSize += PAGE_SIZE;
			if (freeSize == size) {
				curAddr -= size; //return ddr ptr properly
				curAddr += PAGE_SIZE; //since curAddr isnt incremented until the end of the iteration
				return (void*) curAddr;
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

	for(uint32 curAddr = USER_HEAP_START; curAddr < USER_HEAP_MAX; curAddr+=PAGE_SIZE){

		if (!userHeap[userHeapIndex(curAddr)].reserved)
			//free page
			freeSize += PAGE_SIZE;

		else{
			if (freeSize >= size && freeSize < minFreeSize){
				minFreeSize = freeSize;
				startAddress = curAddr - minFreeSize;
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

	int curAddrIndex = userHeapIndex(startAddress);
	while(size > 0){
		userHeap[curAddrIndex].reserved = 1;
		userHeap[curAddrIndex].startAddress = startAddress;
		curAddrIndex++;
		size-=PAGE_SIZE;
	}
	return (void*)userHeap[(curAddrIndex % numOfUserHeapEntries)].virtualAddress;
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
