#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

//file declarations

struct DataLocAndSz {
	uint32 virtualAddr;
	uint32 numBytesAllocated;
};

int dataArrIdx = 0;                                                         //next idx to fill in the data
struct DataLocAndSz dataArr[5000];                                     //array of current kheap memory allocations

uint32 K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR = (uint32)KERNEL_HEAP_START; //next ptr to check to allocate in NEX FIT STRATEGY

void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT 2022 - [1] Kernel Heap] kmalloc()
	// Write your code here, remove the panic and write your code
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");

	//NOTE: Allocation using NEXTFIT strategy
	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
	//refer to the project presentation and documentation for details

	//NEXT FIT STRATEGY START--------------------------------------------------------------------------------------------------------------

	uint32 start = K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR;     //keep track of the point I started in
	uint32 sizeToAllocate = ROUNDUP(size, PAGE_SIZE);
	uint32 end = start + sizeToAllocate; //seems to overflow in some cases

	//cprintf("%d -- %d -- %d -- %d\n", (uint32)start, (uint32)size, (uint32)end, (uint32)KERNEL_HEAP_MAX);

    //TODO need to handle the case of starting over from zero
	if (sizeToAllocate > KERNEL_HEAP_MAX - start)        //not enough space to try in the first place
		return NULL;

    for( ; start < end; start += PAGE_SIZE)
    {
	    struct Frame_Info* frameInfo;

	    int res = allocate_frame(&frameInfo);
	    if(res == E_NO_MEM)
		    return NULL;

	    res = map_frame(ptr_page_directory, frameInfo, (void*)start, PERM_PRESENT | PERM_WRITEABLE);
	    if(res == E_NO_MEM)
	    {
		    free_frame(frameInfo);
		    return NULL;
	    }
    }

   //successful allocation
   //cprintf("in %d\n", dataArrIdx);
   dataArr[dataArrIdx].virtualAddr = K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR;
   dataArr[dataArrIdx].numBytesAllocated = sizeToAllocate;

   K_MALLOC_NEXT_FIT_STRATEGY_CUR_PTR = start; //move the pointer so the next allocation starts right after this one


   return (void*)dataArr[dataArrIdx++].virtualAddr;

	//NEXT FIT STRATEGY END --------------------------------------------------------------------------------------------------------------


	//TODO: [PROJECT 2022 - BONUS1] Implement a Kernel allocation strategy
	// Instead of the Next allocation/deallocation, implement
	// BEST FIT strategy
	// use "isKHeapPlacementStrategyBESTFIT() ..."
	// and "isKHeapPlacementStrategyNEXTFIT() ..."
	//functions to check the current strategy
	//change this "return" according to your answer


}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2022 - [2] Kernel Heap] kfree()
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	//you need to get the size of the given allocation using its address
	//refer to the project presentation and documentation for details

    for(int i=0;i<dataArrIdx;i++)
	{
	    if (virtual_address==(void*)dataArr[i].virtualAddr)
		    {
		   uint32 VA=dataArr[i].virtualAddr+dataArr[i].numBytesAllocated;
		   for(uint32 k= (uint32)virtual_address;k<VA;k+=PAGE_SIZE)
		   {
			unmap_frame(ptr_page_directory,(uint32*)k);
		   }
			dataArr[i].numBytesAllocated=0;
			dataArr[i].virtualAddr=0;
	     }
	}

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

	struct Frame_Info* frameInfo = to_frame_info(physical_address);  //doesnt return an error, only panics

    for (uint32 addr = KERNEL_HEAP_START; addr < KERNEL_HEAP_MAX; addr += PAGE_SIZE)
    {
        struct Frame_Info* tmpFrame = get_frame_info(ptr_page_directory, (void *)addr, NULL);
        if(frameInfo == tmpFrame)
            return addr;
    }
//change this "return" according to your answer
	return 0;

	uint32 frameNum = physical_address / PAGE_SIZE;  //round down

	//search for frame number in all the page tables

	for (uint32 addr = KERNEL_HEAP_START; addr <= KERNEL_HEAP_MAX; addr+= PAGE_SIZE){
	    	uint32* pageTable;
	        get_page_table(ptr_page_directory, (void *)addr, &pageTable);

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

	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT 2022 - [4] Kernel Heap] kheap_physical_address()
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//return the physical address corresponding to given virtual_address
	//refer to the project presentation and documentation for details

	//change this "return" according to your answer

	struct Frame_Info * frameInfo = get_frame_info(ptr_page_directory, (void *)virtual_address, NULL);
	if(frameInfo == NULL)
		return 0;
	return to_physical_address(frameInfo);
}


/*
 *
 */
