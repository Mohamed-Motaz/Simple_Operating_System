#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//helper methods
void initializeDataArr();
void* kernelHeapNextFitStrategy(uint32 size);
void* kernelHeapBestFitStrategy(uint32 size);
uint32 kernelHeapIndex(uint32);
void* allocateInKernelHeap(uint32 startAddress, uint32 size);
void* getFreeSpaceForAllocationInKernelHeap();

//declarations
struct kernelHeapEntry {
	uint32 startAddress;      //same for all parts in a segment
	uint32 virtualAddress;    //real one, it doesn't change ever
	uint32 physicalAddress;
	bool reserved;
};
int numOfKernelHeapEntries = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE;
struct kernelHeapEntry kernelHeap[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE];                                 //array of current kheap memory allocations
uint32 KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)KERNEL_HEAP_START;      //next ptr to check to allocate in NEX FIT STRATEGY
bool kernelHeapIntialized = 0;

void initializeDataArr(){
    //initialize the kernelHeap by setting all the addresses correctly
	for (uint32 curAddr = KERNEL_HEAP_START; curAddr < KERNEL_HEAP_MAX; curAddr += PAGE_SIZE){
		kernelHeap[kernelHeapIndex(curAddr)].reserved = 0;
		kernelHeap[kernelHeapIndex(curAddr)].startAddress = 0;
		kernelHeap[kernelHeapIndex(curAddr)].virtualAddress = curAddr;

	}
}


uint32 kernelHeapIndex(uint32 curAddr){
	return (curAddr - KERNEL_HEAP_START) / PAGE_SIZE;
}

void* getFreeSpaceForAllocationInKernelHeap(uint32 start,uint32 end,uint32 size){

	uint32 freeSize = 0;

	for (uint32 curAddr = start;curAddr < end; curAddr += PAGE_SIZE) {
		if (!kernelHeap[kernelHeapIndex(curAddr)].reserved) {
			//found a free page
			freeSize += PAGE_SIZE;

			if (freeSize == size) {
				curAddr -= size;	//return curAddr ptr properly
				curAddr += PAGE_SIZE; //since curAddr isnt incremented until the end of the iteration
				return (void*) curAddr;
			}
		} else
			//need to look for another segment
			freeSize = 0;
	}
	return NULL;
}


void* kernelHeapNextFitStrategy(uint32 size){

	void* freeSizePtr = getFreeSpaceForAllocationInKernelHeap(
			KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR, KERNEL_HEAP_MAX, size);

	freeSizePtr =(freeSizePtr == NULL) ?
		getFreeSpaceForAllocationInKernelHeap(KERNEL_HEAP_START,KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR, size) :freeSizePtr;

	KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (freeSizePtr != NULL) ?
		(uint32) allocateInKernelHeap((uint32) freeSizePtr, size) : KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR;

	return freeSizePtr;

}

void* kernelHeapBestFitStrategy(uint32 size){

	uint32 freeSize = 0;
	uint32 minFreeBytesSoFar = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) + PAGE_SIZE; //just over the amount of kheap
	uint32 minFreeBytesPtr = 0;

	for(uint32 curAddr = KERNEL_HEAP_START; curAddr < KERNEL_HEAP_MAX; curAddr+= PAGE_SIZE){
		if (!kernelHeap[kernelHeapIndex(curAddr)].reserved)
			//free page
			freeSize += PAGE_SIZE;
		else{
			if (freeSize >= size && freeSize < minFreeBytesSoFar){
				minFreeBytesSoFar = freeSize;
				minFreeBytesPtr = curAddr - minFreeBytesSoFar;
			}
			freeSize = 0;
		}
	}
	//no space found
	if (minFreeBytesPtr == 0){
		//this condition is for when I reach the K_HEAP_MAX without breaking
		if (freeSize >= size && freeSize < minFreeBytesSoFar){
			minFreeBytesSoFar = freeSize;
			minFreeBytesPtr = KERNEL_HEAP_MAX - minFreeBytesSoFar;
		}
		else
			return NULL;
	}

	allocateInKernelHeap(minFreeBytesPtr,size);
	return (void*)minFreeBytesPtr;
}


void* allocateInKernelHeap(uint32 startAddress, uint32 size){

	int curAddressIndex = kernelHeapIndex(startAddress);
	while(size){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)kernelHeap[curAddressIndex].virtualAddress, PERM_PRESENT | PERM_WRITEABLE);
		kernelHeap[curAddressIndex].reserved = 1;
		kernelHeap[curAddressIndex].startAddress = startAddress;
		kernelHeap[curAddressIndex].physicalAddress = kheap_physical_address(kernelHeap[curAddressIndex].virtualAddress);
		curAddressIndex++;
		size-=PAGE_SIZE;
	}
	return (void*)kernelHeap[(curAddressIndex % numOfKernelHeapEntries)].virtualAddress;

}

void* kmalloc(unsigned int size)
{
	//TODO:DONE [PROJECT 2022 - [1] Kernel Heap] kmalloc()
    if (!kernelHeapIntialized){
        initializeDataArr();
        kernelHeapIntialized = 1;
    }


	size = ROUNDUP(size, PAGE_SIZE); //round up the size before passing it to the methods
	//NEXT FIT STRATEGY START--------------------------------------------------------------------------------------------------------------

	if (isKHeapPlacementStrategyNEXTFIT())
	    return kernelHeapNextFitStrategy(size);

	//NEXT FIT STRATEGY END --------------------------------------------------------------------------------------------------------------
    if (isKHeapPlacementStrategyBESTFIT())
    	return kernelHeapBestFitStrategy(size);


	//TODO:DONE [PROJECT 2022 - BONUS1] Implement a Kernel allocation strategy
    return NULL;

}

void kfree(void* virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [2] Kernel Heap] kfree()
	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);

	for (uint32 curAddr = (uint32)virtual_address; curAddr < KERNEL_HEAP_MAX; curAddr += PAGE_SIZE){
		int addressIndex = kernelHeapIndex(curAddr);
		if (kernelHeap[addressIndex].reserved && (uint32)virtual_address == kernelHeap[addressIndex].startAddress){
			unmap_frame(ptr_page_directory, (void*)curAddr);
			kernelHeap[addressIndex].reserved = 0;
			kernelHeap[addressIndex].startAddress = 0;  //return the value to be the same during initialization
			kernelHeap[addressIndex].physicalAddress = 0;
		}
		else
			break;
	}
}


unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO:DONE [PROJECT 2022 - [3] Kernel Heap] kheap_virtual_address()

	for (int curAddressIndex = 0;  curAddressIndex < numOfKernelHeapEntries; curAddressIndex++){
		if (kernelHeap[curAddressIndex].physicalAddress == physical_address)
			return kernelHeap[curAddressIndex].virtualAddress;
	}
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [4] Kernel Heap] kheap_physical_address()

    uint32 * pageTable = NULL;
    get_page_table(ptr_page_directory, (void *)virtual_address, &pageTable);

    if (pageTable == NULL)
    	return 0;

    uint32 physicalAddress = pageTable[PTX(virtual_address)] >> 12;
    physicalAddress *= PAGE_SIZE;

    //extract offset from the va
    uint32 offset = (virtual_address) & ~(~0 << 12);

    //now attempt to add the offset
    physicalAddress |= offset;

    return physicalAddress;
}
