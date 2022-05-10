#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//helper methods
void initializeDataArr();
void* kmallocNextFit(uint32 size);
void* kmallocBestFit(uint32 size);
uint32 min(uint32, uint32);
uint32 dataArrIdx(uint32);
//declarations
struct DataLocAndSz {
	uint32 virtualAddr;      //same for all parts in a segment
	uint32 realVirtualAddr; //real one, it doesn't change ever
	uint32 numBytesAllocated;
	uint32 physAddr;
};
int dataArrSz = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE + 100;
struct DataLocAndSz dataArr[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE + 100];                                 //array of current kheap memory allocations
uint32 K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)KERNEL_HEAP_START;      //next ptr to check to allocate in NEX FIT STRATEGY
uint8 firstRun = 1;

void* kmalloc(unsigned int size)
{
	//TODO:DONE [PROJECT 2022 - [1] Kernel Heap] kmalloc()
    if (firstRun){
        initializeDataArr();
        firstRun = 0;
    }


	size = ROUNDUP(size, PAGE_SIZE); //round up the size before passing it to the methods
	//NEXT FIT STRATEGY START--------------------------------------------------------------------------------------------------------------

	if (isKHeapPlacementStrategyNEXTFIT())
	    return kmallocNextFit(size);

	//NEXT FIT STRATEGY END --------------------------------------------------------------------------------------------------------------
    if (isKHeapPlacementStrategyBESTFIT())
    	return kmallocBestFit(size);

	//TODO:DONE [PROJECT 2022 - BONUS1] Implement a Kernel allocation strategy
    return NULL;
}

void initializeDataArr(){
    //initialize the dataArr by setting all the addresses correctly
	for (uint32 addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_MAX; addr += PAGE_SIZE){
		dataArr[dataArrIdx(addr)].numBytesAllocated = 0;
		dataArr[dataArrIdx(addr)].virtualAddr = addr;
		dataArr[dataArrIdx(addr)].realVirtualAddr = addr;

	}
}

uint32 min(uint32 a, uint32 b){
	if (a <= b)
		return a;
	return b;
}

uint32 dataArrIdx(uint32 addr){
	return (addr - KERNEL_HEAP_START) / PAGE_SIZE;
}

void* kmallocNextFit(uint32 size){
	uint32 curAddr = K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR;
	uint32 end = curAddr;     //end is start as path may be circular
	uint32 freeBytesFound = 0;

	for ( ;curAddr < KERNEL_HEAP_MAX; curAddr += PAGE_SIZE){
		if (dataArr[dataArrIdx(curAddr)].numBytesAllocated == 0){
			//found a free page
			freeBytesFound += PAGE_SIZE;

			if (freeBytesFound == size){
				curAddr -= size;//return curAddr ptr properly
				curAddr += PAGE_SIZE; //since curAddr isnt incremented until the end of the iteration
				break;
			}
            continue;
		}
		//need to look for another segment
		freeBytesFound = 0;
	}

	//retry from the start of the heap
	if (freeBytesFound != size){
		curAddr = KERNEL_HEAP_START;
		freeBytesFound = 0;
		for ( ;curAddr < end; curAddr += PAGE_SIZE){
			if (0 == dataArr[dataArrIdx(curAddr)].numBytesAllocated){
				//found a free page
				freeBytesFound += PAGE_SIZE;

				if (freeBytesFound == size){
					curAddr -= size; //return curAddr ptr properly
					curAddr += PAGE_SIZE;  //since curAddr isnt incremented until the end of the iteration
					break;
				}
				continue;
			}
			//need to look for another segment
			freeBytesFound = 0;
		}
	}

	if (freeBytesFound < size)return NULL;

	//begin the allocation
	uint32 startAlloc = curAddr;
	uint32 szAlloc = 0;
	for (; szAlloc < size; szAlloc += PAGE_SIZE, startAlloc += PAGE_SIZE){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)startAlloc, PERM_PRESENT | PERM_WRITEABLE);
		dataArr[dataArrIdx(startAlloc)].numBytesAllocated = freeBytesFound;
		dataArr[dataArrIdx(startAlloc)].virtualAddr = curAddr;
		dataArr[dataArrIdx(startAlloc)].physAddr = kheap_physical_address(startAlloc);
	}

	K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR = curAddr + szAlloc;  //set the pointer to look just after this point of allocation

	return (void*)curAddr;
}

void* kmallocBestFit(uint32 size){
	uint32 curAddr = KERNEL_HEAP_START;
	uint32 end = KERNEL_HEAP_MAX;
	uint32 freeBytesFound = 0;
	uint32 minFreeBytesSoFar = end - curAddr + 7; //just over the amount of kheap
    uint32 minFreeBytesPtr = -1;
    uint32 spaceFound = 0;

	for( ;curAddr < end; curAddr += PAGE_SIZE){
		if (dataArr[dataArrIdx(curAddr)].numBytesAllocated == 0)
			//free page
			freeBytesFound += PAGE_SIZE;

		else{
			if (freeBytesFound >= size && freeBytesFound < minFreeBytesSoFar){
				minFreeBytesSoFar = freeBytesFound;
				minFreeBytesPtr = curAddr - minFreeBytesSoFar;
				spaceFound = 1;
			}
			freeBytesFound = 0;
		}
	}
	//this condition is for when I reach the K_HEAP_MAX without breaking
	if (freeBytesFound >= size && freeBytesFound < minFreeBytesSoFar){
		minFreeBytesSoFar = freeBytesFound;
		minFreeBytesPtr = curAddr - minFreeBytesSoFar;
		spaceFound = 1;
	}

	//no space found
	if (spaceFound == 0)
		return NULL;

	//begin the allocation
	uint32 startAlloc = minFreeBytesPtr;
	uint32 szAlloc = 0;
	for (; szAlloc < size; szAlloc += PAGE_SIZE, startAlloc += PAGE_SIZE){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)startAlloc, PERM_PRESENT | PERM_WRITEABLE);
		dataArr[dataArrIdx(startAlloc)].numBytesAllocated = size;
		dataArr[dataArrIdx(startAlloc)].virtualAddr = minFreeBytesPtr;
		dataArr[dataArrIdx(startAlloc)].physAddr = kheap_physical_address(startAlloc);
	}

	return (void*)minFreeBytesPtr;
}

void kfree(void* virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [2] Kernel Heap] kfree()
	uint32 startDeAlloc = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);

	for (; startDeAlloc < KERNEL_HEAP_MAX; startDeAlloc += PAGE_SIZE)
		if (0 != dataArr[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].numBytesAllocated &&
	       (uint32)virtual_address == dataArr[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].virtualAddr){
			unmap_frame(ptr_page_directory, (void*)startDeAlloc);
			dataArr[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].numBytesAllocated = 0;
			dataArr[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].virtualAddr = startDeAlloc;  //return the value to be the same during initialization
			dataArr[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].physAddr = 0;
		}else
			break;

}


unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO:DONE [PROJECT 2022 - [3] Kernel Heap] kheap_virtual_address()

	for (int i = 0; i < dataArrSz; i++){
		if (dataArr[i].physAddr == physical_address)
			return dataArr[i].realVirtualAddr;
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

    uint32 physAddr = pageTable[PTX(virtual_address)] >> 12;
    physAddr *= PAGE_SIZE;

    //extract offset from the va
    uint32 offset = (virtual_address) & ~(~0 << 12);

    //now attempt to add the offset
    physAddr |= offset;

    return physAddr;
}


/*
 *
 */
