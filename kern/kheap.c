#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//helper methods
void initializeDataArr();
void* kernelHeapNextFitStrategy(uint32 size);
void* kernelHeapBestFitStrategy(uint32 size);
uint32 min(uint32, uint32);
uint32 kernelHeapIndex(uint32);
void allocateInKernelHeap(uint32 startAddress, uint32 size);
//declarations
struct kernelHeapEntry {
	uint32 startAddress;      //same for all parts in a segment
	uint32 virtualAddress; //real one, it doesn't change ever
	bool used;
	uint32 physicalAddress;
};
int numOfKernelHeapEntries = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE;
struct kernelHeapEntry kernelHeap[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE];                                 //array of current kheap memory allocations
uint32 KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)KERNEL_HEAP_START;      //next ptr to check to allocate in NEX FIT STRATEGY
bool kernelHeapIntialized = 0;

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

void initializeDataArr(){
    //initialize the kernelHeap by setting all the addresses correctly
	for (uint32 addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_MAX; addr += PAGE_SIZE){
		kernelHeap[kernelHeapIndex(addr)].used = 0;
		kernelHeap[kernelHeapIndex(addr)].startAddress = 0;
		kernelHeap[kernelHeapIndex(addr)].virtualAddress = addr;

	}
}

uint32 min(uint32 a, uint32 b){
	if (a <= b)
		return a;
	return b;
}

uint32 kernelHeapIndex(uint32 addr){
	return (addr - KERNEL_HEAP_START) / PAGE_SIZE;
}

void* kernelHeapNextFitStrategy(uint32 size){
	uint32 freeSize = 0;

	for (uint32 curAddr = KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR ;curAddr < KERNEL_HEAP_MAX; curAddr += PAGE_SIZE){
		if (!kernelHeap[kernelHeapIndex(curAddr)].used){
			//found a free page
			freeSize += PAGE_SIZE;

			if (freeSize == size){
				curAddr -= size;//return curAddr ptr properly
				curAddr += PAGE_SIZE; //since curAddr isnt incremented until the end of the iteration
				allocateInKernelHeap(curAddr,size);
				KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = curAddr + size;
				return (void*)curAddr;
			}
		}
		else
		//need to look for another segment
		freeSize = 0;
	}

	//retry from the start of the heap
	freeSize = 0;

	for (uint32 curAddr = KERNEL_HEAP_START;curAddr < KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR; curAddr += PAGE_SIZE){

		if (!kernelHeap[kernelHeapIndex(curAddr)].used){
			//found a free page
			freeSize += PAGE_SIZE;
			if (freeSize == size){
				curAddr -= size; //return curAddr ptr properly
				curAddr += PAGE_SIZE;  //since curAddr isnt incremented until the end of the iteration
				allocateInKernelHeap(curAddr,size);
				KERNEL_HEAP_NEXT_FIT_STRATEGY_CUR_PTR = curAddr + size;
				return (void*)curAddr;
			}
		}
		else
			//need to look for another segment
			freeSize = 0;
	}
	return NULL;
}

void* kernelHeapBestFitStrategy(uint32 size){

	uint32 freeSize = 0;
	uint32 minFreeBytesSoFar = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) + 7; //just over the amount of kheap
	uint32 minFreeBytesPtr = 0;

	for(uint32 curAddr = KERNEL_HEAP_START; curAddr < KERNEL_HEAP_MAX; curAddr+=PAGE_SIZE){
		if (!kernelHeap[kernelHeapIndex(curAddr)].used)
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

void allocateInKernelHeap(uint32 startAddress, uint32 size){

	int addrIndex = kernelHeapIndex(startAddress);
	while(size){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)kernelHeap[addrIndex].virtualAddress, PERM_PRESENT | PERM_WRITEABLE);
		kernelHeap[addrIndex].used = 1;
		kernelHeap[addrIndex].startAddress = startAddress;
		kernelHeap[addrIndex].physicalAddress = kheap_physical_address(kernelHeap[addrIndex].virtualAddress);
		addrIndex++;
		size-=PAGE_SIZE;
	}
}

void kfree(void* virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [2] Kernel Heap] kfree()
	virtual_address = ROUNDDOWN(virtual_address,PAGE_SIZE);

	for (uint32 addr = (uint32)virtual_address; addr < KERNEL_HEAP_MAX; addr += PAGE_SIZE){
		int addressIndex = kernelHeapIndex(addr);
		if (kernelHeap[addressIndex].used && (uint32)virtual_address == kernelHeap[addressIndex].startAddress){
			unmap_frame(ptr_page_directory, (void*)addr);
			kernelHeap[addressIndex].used = 0;
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

	for (int i = 0; i < numOfKernelHeapEntries; i++){
		if (kernelHeap[i].physicalAddress == physical_address)
			return kernelHeap[i].virtualAddress;
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
