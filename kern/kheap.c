#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//helper methods
void initializeDataArr();
void* kmallocNextFit(uint32 size);
void* kmallocBestFit(uint32 size);
uint32 min(uint32, uint32);
uint32 kHeapIdx(uint32);
void allocateInKHeap(uint32 startAddress, uint32 size);
//declarations
struct DataLocAndSz {
	uint32 virtualAddr;      //same for all parts in a segment
	uint32 realVirtualAddr; //real one, it doesn't change ever
	uint32 numBytesAllocated;
	uint32 physAddr;
};
int kHeapSz = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE + 100;
struct DataLocAndSz kHeap[(KERNEL_HEAP_MAX - KERNEL_HEAP_START) / PAGE_SIZE + 100];                                 //array of current kheap memory allocations
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
    //initialize the kHeap by setting all the addresses correctly
	for (uint32 addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_MAX; addr += PAGE_SIZE){
		kHeap[kHeapIdx(addr)].numBytesAllocated = 0;
		kHeap[kHeapIdx(addr)].virtualAddr = addr;
		kHeap[kHeapIdx(addr)].realVirtualAddr = addr;

	}
}

uint32 min(uint32 a, uint32 b){
	if (a <= b)
		return a;
	return b;
}

uint32 kHeapIdx(uint32 addr){
	return (addr - KERNEL_HEAP_START) / PAGE_SIZE;
}

void* kmallocNextFit(uint32 size){
	uint32 curAddr = K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR;
	uint32 end = curAddr;     //end is start as path may be circular
	uint32 freeSize = 0;

	for ( ;curAddr < KERNEL_HEAP_MAX; curAddr += PAGE_SIZE){
		if (kHeap[kHeapIdx(curAddr)].numBytesAllocated == 0){
			//found a free page
			freeSize += PAGE_SIZE;

			if (freeSize == size){
				curAddr -= size;//return curAddr ptr properly
				curAddr += PAGE_SIZE; //since curAddr isnt incremented until the end of the iteration
				break;
			}
            continue;
		}
		//need to look for another segment
		freeSize = 0;
	}

	//retry from the start of the heap
	if (freeSize != size){
		curAddr = KERNEL_HEAP_START;
		freeSize = 0;
		for ( ;curAddr < end; curAddr += PAGE_SIZE){
			if (0 == kHeap[kHeapIdx(curAddr)].numBytesAllocated){
				//found a free page
				freeSize += PAGE_SIZE;

				if (freeSize == size){
					curAddr -= size; //return curAddr ptr properly
					curAddr += PAGE_SIZE;  //since curAddr isnt incremented until the end of the iteration
					break;
				}
				continue;
			}
			//need to look for another segment
			freeSize = 0;
		}
	}

	if (freeSize < size)return NULL;

	//begin the allocation
	allocateInKHeap(curAddr,size);

	K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR = curAddr + size;  //set the pointer to look just after this point of allocation

	return (void*)curAddr;
}

void* kmallocBestFit(uint32 size){

	uint32 freeSize = 0;
	uint32 minFreeBytesSoFar = (KERNEL_HEAP_MAX - KERNEL_HEAP_START) + 7; //just over the amount of kheap
	uint32 minFreeBytesPtr = 0;

	for(uint32 curAddr = KERNEL_HEAP_START; curAddr < KERNEL_HEAP_MAX; curAddr+=PAGE_SIZE){
		if (kHeap[kHeapIdx(curAddr)].numBytesAllocated == 0)
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

	allocateInKHeap(minFreeBytesPtr,size);
	return (void*)minFreeBytesPtr;
}

void allocateInKHeap(uint32 startAddress, uint32 size){

	int addrIndex = kHeapIdx(startAddress);
	while(size > 0){
		struct Frame_Info *ptr_frame_info = 0;
		allocate_frame(&ptr_frame_info);
		map_frame(ptr_page_directory, ptr_frame_info, (void*)kHeap[addrIndex].realVirtualAddr, PERM_PRESENT | PERM_WRITEABLE);
		kHeap[addrIndex].numBytesAllocated = 1;
		kHeap[addrIndex].virtualAddr = startAddress;
		kHeap[addrIndex].physAddr = kheap_physical_address(kHeap[addrIndex].realVirtualAddr);
		addrIndex++;
		size-=PAGE_SIZE;
	}
}

void kfree(void* virtual_address)
{
	//TODO:DONE [PROJECT 2022 - [2] Kernel Heap] kfree()
	uint32 startDeAlloc = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);

	for (; startDeAlloc < KERNEL_HEAP_MAX; startDeAlloc += PAGE_SIZE)
		if (0 != kHeap[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].numBytesAllocated &&
	       (uint32)virtual_address == kHeap[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].virtualAddr){
			unmap_frame(ptr_page_directory, (void*)startDeAlloc);
			kHeap[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].numBytesAllocated = 0;
			kHeap[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].virtualAddr = startDeAlloc;  //return the value to be the same during initialization
			kHeap[(startDeAlloc - KERNEL_HEAP_START) / PAGE_SIZE].physAddr = 0;
		}else
			break;

}


unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO:DONE [PROJECT 2022 - [3] Kernel Heap] kheap_virtual_address()

	for (int i = 0; i < kHeapSz; i++){
		if (kHeap[i].physAddr == physical_address)
			return kHeap[i].realVirtualAddr;
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
