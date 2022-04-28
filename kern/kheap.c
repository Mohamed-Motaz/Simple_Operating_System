#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

int K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX = 0;

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2022 - [1] Kernel Heap] kmalloc()
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	//NOTE: Allocation using NEXTFIT strategy
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project presentation and documentation for details

	//NEXT FIT STRATEGY START--------------------------------------------------------------------------------------------------------------

	//loop on kernel heap, page tables 960 to 1023

	for (int i = 960; i < 1024; i++){
		uint32 pageTable = ptr_page_directory[i];
		kheap_virtual_address()
	}

	//NEXT FIT STRATEGY END --------------------------------------------------------------------------------------------------------------


	//TODO: [PROJECT 2022 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the Next allocation/deallocation, implement
	// BEST FIT strategy
	// use "isKHeapPlacementStrategyBESTFIT() ..."
	// and "isKHeapPlacementStrategyNEXTFIT() ..."
	//functions to check the current strategy
	//change this "return" according to your answer

	return NULL;


}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2022 - [2] Kernel Heap] kfree()
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT 2022 - [3] Kernel Heap] kheap_virtual_address()
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//return the virtual address corresponding to given physical_address
	//refer to the project presentation and documentation for details

	//change this "return" according to your answer

	//extract frame number

	uint32 frameNum = physical_address / PAGE_SIZE;  //round down

	//search for frame number in all the page tables

	for (uint32 addr = KERNEL_HEAP_START; addr <= KERNEL_HEAP_MAX; addr+= PAGE_SIZE){
	    	uint32* pageTable;
	        get_page_table(ptr_page_directory, (void *)addr, 0, &pageTable);

	        if (pageTable == NULL)
	        	continue;

	        for (int i = 0; i < 1024; i++){

	        	if ((pageTable[i] >> 12) == frameNum){
	        		if ((pageTable[i] & PERM_PRESENT) == 0)  //not present
	        			continue;

	            	addr = addr >> 12; //remove the offset
	                addr |= i; //add the page
	                addr = addr << 12; //re-add the offset
	            	//cprintf("%x \n", addr);
	        		return (uint32)(addr);

	        	}
	        }
	    }

	return NULL;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2022 - [4] Kernel Heap] kheap_physical_address()
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//change this "return" according to your answer
	return 0;
}


/*
 * int numFramesToAllocate = ROUNDUP(size, PAGE_SIZE) / PAGE_SIZE;
    int startingIdx = K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX; //to prevent an infinite loop
    int numFramesFreeSoFar = 0;
    int firstFreeFrameMetIdx = -1;
    do{
    	 //TODO: optimize this for loop

		struct Frame_Info * curFrame = frames_info[K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX];
		if (curFrame->references != 0){
			//I can't allocate this frame
            numFramesFreeSoFar = 0;
            firstFreeFrameMetIdx = -1;
		}else{
            if (numFramesFreeSoFar == 0) //first free frame met so far
            	firstFreeFrameMetIdx = K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX;

			numFramesFreeSoFar++;
		}

        if (numFramesFreeSoFar == numFramesToAllocate)
        	break;


		K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX++;

		if (K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX == number_of_frames){
			//I reached the end, I have to restart and set numFramesFreeSoFar = 0
			numFramesFreeSoFar = 0;
			firstFreeFrameMetIdx = -1;
		}

		K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX %= number_of_frames; //to go back to idx 0
    }while(startingIdx != K_MALLOC_NEXT_FIT_STRATEGY_CUR_IDX);

    if (numFramesFreeSoFar == numFramesToAllocate){
    	//success, need to allocate
    	for (int i = firstFreeFrameMetIdx; i < numFramesToAllocate; i++){
    		struct Frame_Info * frameInfo = frames_info[i];
    		allocate_frame(&frameInfo);
    	}
    }
 */
 */
