#include <inc/lib.h>


void intializeUserHeap();
void* userHeapNextFitStrategy();
void* userHeapBestFitStrategy();
void* allocateInUserHeap();
uint32 getSizeForExpand();
void freeFromUserHeap();
uint32 userHeapIndex();
uint32 absoluteValue();
uint32 getReservedBlockSize();

struct userHeapEntry{
	uint32 virtualAddress;
	uint32 startAddress;
	bool used;
};
int numOfUserHeapEntries = (USER_HEAP_MAX - USER_HEAP_START)/ PAGE_SIZE;
struct userHeapEntry userHeap[(USER_HEAP_MAX - USER_HEAP_START)/ PAGE_SIZE];
uint32 USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = USER_HEAP_START;
bool userHeapIntialized = 0;


uint32 userHeapIndex(uint32 addr){
	return (addr - USER_HEAP_START) / PAGE_SIZE;
}

uint32 absoluteValue(uint32 value){
	return (value<0)? (value*-1) : value;
}

void intializeUserHeap(){

	for (uint32 addr = USER_HEAP_START; addr < USER_HEAP_MAX; addr += PAGE_SIZE){
		int addrIndex = userHeapIndex(addr);
		userHeap[addrIndex].startAddress = 0;
		userHeap[addrIndex].virtualAddress = addr;
		userHeap[addrIndex].used = 0;
	}
}
void* allocateInUserHeap(uint32 startAddress, uint32 size){

	int addrIndex = userHeapIndex(startAddress);
	while(size > 0){
		userHeap[addrIndex].used = 1;
		userHeap[addrIndex].startAddress = startAddress;
		addrIndex++;
		size-=PAGE_SIZE;
	}
	return (void*)userHeap[(addrIndex % numOfUserHeapEntries)].virtualAddress;
}

uint32 getSizeForExpand(uint32 virtual_address){
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
void freeFromUserHeap(void* virtual_address,uint32 size){
	for(uint32 addr = (uint32)virtual_address; addr < (uint32)virtual_address + size; addr += PAGE_SIZE ){
		int addrIndex = userHeapIndex(addr);
		userHeap[addrIndex].used=0;
		userHeap[addrIndex].startAddress=0;
	}
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

void* userHeapNextFitStrategy(uint32 size){

	uint32 freeSize = 0;

	for(uint32 addr = USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR; addr < USER_HEAP_MAX; addr+= PAGE_SIZE){

		if (!userHeap[userHeapIndex(addr)].used){
			//found a free page
			freeSize += PAGE_SIZE;
			if (freeSize == size){
				addr -= size;//return ddr ptr properly
				addr += PAGE_SIZE; //since addr isnt incremented until the end of the iteration
				USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)allocateInUserHeap(addr, size);
				return (void*)addr;
			}
		}
		else
			//need to look for another segment
			freeSize = 0;
	}
	freeSize = 0;
	for(uint32 addr = USER_HEAP_START; addr < USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR; addr+= PAGE_SIZE){
		if (!userHeap[userHeapIndex(addr)].used){
			//found a free page
			freeSize += PAGE_SIZE;
			if (freeSize == size){
				addr -= size;//return addr ptr properly
				addr += PAGE_SIZE; //since addr isnt incremented until the end of the iteration
				USER_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)allocateInUserHeap(addr, size);
				return (void*)addr;
			}
		}
		else
			//need to look for another segment
			freeSize = 0;
	}
	return NULL;
}
void* userHeapBestFitStrategy(uint32 size){

	uint32 freeSize = 0;
	uint32 minFreeSize = (USER_HEAP_MAX - USER_HEAP_START) + 7; //just over the amount of userHeap
	uint32 startAddress = 0;

	for(uint32 addr = USER_HEAP_START; addr < USER_HEAP_MAX; addr+=PAGE_SIZE){

		if (!userHeap[userHeapIndex(addr)].used)
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


// malloc()
//	This function use NEXT FIT strategy to allocate space in heap
//  with the given size and return void pointer to the start of the allocated space

//	To do this, we need to switch to the kernel, allocate the required space
//	in Page File then switch back to the user again.
//
//	We can use sys_allocateMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls allocateMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the allocateMem function is empty, make sure to implement it.


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

void* malloc(uint32 size)
{
	//TODO:DONE [PROJECT 2022 - [9] User Heap malloc()] [User Side]

	if(!userHeapIntialized){
		intializeUserHeap();
		userHeapIntialized = 1;
	}
	size = ROUNDUP(size, PAGE_SIZE);

	if(!size) return NULL;

	void* address = (sys_isUHeapPlacementStrategyNEXTFIT())?userHeapNextFitStrategy(size):userHeapBestFitStrategy(size);
	if(address != NULL){
		sys_allocateMem((uint32)address,size);
	}
	return address;
	// Steps:
	//	1) Implement NEXT FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	//	2) if no suitable space found, return NULL
	//	 Else,
	//	3) Call sys_allocateMem to invoke the Kernel for allocation
	// 	4) Return pointer containing the virtual address of allocated space,
	//

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyNEXTFIT() and
	//sys_isUHeapPlacementStrategyBESTFIT() for the bonus
	//to check the current strategy

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

// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from page file and main memory then switch back to the user again.
//
//	We can use sys_freeMem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls freeMem(struct Env* e, uint32 virtual_address, uint32 size) in
//		"memory_manager.c", then switch back to the user mode here
//	the freeMem function is empty, make sure to implement it.

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

	//you shold get the size of the given allocation using its address
	//you need to call sys_freeMem()
	//refer to the project presentation and documentation for details
}


void sfree(void* virtual_address)
{
	panic("sfree() is not requried ..!!");
}


//===============
// [2] realloc():
//===============

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_moveMem(uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		which switches to the kernel mode, calls moveMem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
//		in "memory_manager.c", then switch back to the user mode here
//	the moveMem function is empty, make sure to implement it.

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
	uint32 freeSize = getSizeForExpand((uint32)virtual_address+oldSize);

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
