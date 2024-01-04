#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#include <inc/environment_definitions.h>
#include "../cpu/sched.h"
#include "../proc/user_environment.h"
//#include "working_set_manager.h"

void* FreeRAM(uint32 size)
{
	struct Env* iter;
	for(int i = 0; i < num_of_ready_queues - 1; i++)
	{
		LIST_FOREACH(iter, &(env_ready_queues[i]))
			if(iter->env_status == ENV_EXIT)
			{
				env_free(iter);
				return kmalloc(size);
			}
	}

	int required_size = size;

	uint32 queue_index = 0, env_index = 0;

	for(int i = 0; i < num_of_ready_queues - 1; i++)
	{
		queue_index = i;

		uint32 temp = 0;
		LIST_FOREACH(iter, &(env_ready_queues[i]))
		{
			env_index = temp;
			if(iter->SecondList.size)
			{
				if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
					required_size -= sizeof(struct WorkingSetElement);
				else
					required_size -= PAGE_SIZE;
			}
			if(required_size <= 0)
				break;
			temp++;
		}
	}

	for(int i = 0; i < num_of_ready_queues - 1; i++)
	{
		uint32 temp = 0;
		LIST_FOREACH(iter,&(env_ready_queues[i]))
		{
			if(iter->SecondList.size)
				env_page_ws_invalidate(iter, iter->SecondList.lh_last->virtual_address);
			if(temp == env_index && i == queue_index)
				return kmalloc(size);
			temp++;
		}
	}

	return NULL;
}

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
//	cprintf("initialize_kheap_dynamic_allocator\n");
	// size with meta
	start = daStart;
	hard_limit = daLimit;
	segment_break = daStart + initSizeToAllocate;
	if (segment_break > hard_limit)
	{
		return E_NO_MEM;
	}
	// virtual address -> page
	uint32 size_without_meta;
	//	size_without_meta = initSizeToAllocate - sizeOfMetaData();
	uint32 no_pages;
	no_pages = ROUNDUP(initSizeToAllocate, PAGE_SIZE); no_pages /= PAGE_SIZE ;
	uint32 page_address = start;
	uint32 *ptr_page_table = NULL;
	uint32 check_allocation = 0;
	uint32 pages = no_pages;


	while(no_pages--)
	{
		ptr_page_table = boot_get_page_table(ptr_page_directory, page_address, 1);
		struct FrameInfo *pointer_frame_info = (get_frame_info(ptr_page_directory,page_address ,&ptr_page_table));
		int allocate_return = allocate_frame(&pointer_frame_info);
		allocate_return = map_frame(ptr_page_directory , pointer_frame_info , page_address, PERM_USER | PERM_PRESENT | PERM_WRITEABLE);
		pointer_frame_info->va = page_address;
		check_allocation ++;
		page_address += PAGE_SIZE;
	}

	initialize_dynamic_allocator(start, initSizeToAllocate);


	if (check_allocation == pages)
	{
		return 0;
	}

	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
	//panic("not implemented yet");
	return E_NO_MEM;
}

void* sbrk(int increment)
{
//	cprintf("sbrk\n");
	uint32 previous_break = segment_break;
	if (increment == 0)
	{
//		cprintf("\nsegment_break = %u\n",segment_break);
		return (void*) segment_break;
	}
	else if (increment > 0)
	{
		if (ROUNDUP(segment_break + increment,PAGE_SIZE) > hard_limit)
		{
			void* ret = FreeRAM(increment - sizeOfMetaData());
			if(ret == NULL)
				panic("increment value exceeds the hard limit ");
			return ret;
		}

		uint32 *ptr_page_table = NULL;

		uint32 new_break = ROUNDUP(segment_break + increment, PAGE_SIZE);

		uint32 no_pages = (new_break - ROUNDUP(previous_break,PAGE_SIZE) ) / PAGE_SIZE;

		segment_break = new_break;

		uint32 page_address = ROUNDUP(previous_break, PAGE_SIZE);

		while(no_pages != 0)
		{
			ptr_page_table = boot_get_page_table(ptr_page_directory, page_address, 1);
			struct FrameInfo *pointer_frame_info = (get_frame_info(ptr_page_directory,page_address ,&ptr_page_table));
			int allocate_return = allocate_frame(&pointer_frame_info);
			allocate_return = map_frame(ptr_page_directory , pointer_frame_info , page_address,PERM_USER | PERM_PRESENT | PERM_WRITEABLE);
			pointer_frame_info->va = page_address;
			no_pages--;
			page_address += PAGE_SIZE;
		}


		return (void*)previous_break;
	}
	else
	{
		uint32 decrement = increment * (-1);

		if(segment_break - decrement < start)
			panic("INVALID SIZE TO DECREMENT (OUT OF KERNAL HEAP)\n");


		uint32 last_allocated_page = ROUNDDOWN(segment_break,PAGE_SIZE);
		uint32 last_needed_page_to_be_allocated =  ROUNDDOWN(segment_break - decrement,PAGE_SIZE);
		uint32 no_pages_dealloc = ( last_allocated_page - last_needed_page_to_be_allocated ) / PAGE_SIZE;
		if((segment_break - decrement) % PAGE_SIZE == 0 && !(segment_break % PAGE_SIZE == 0 && (segment_break - decrement) % PAGE_SIZE == 0))
			no_pages_dealloc++;

		uint32 page_address = ROUNDDOWN(segment_break,PAGE_SIZE);

		segment_break -= decrement;

		while(no_pages_dealloc-- )
		{
			unmap_frame(ptr_page_directory,page_address);
			page_address -= PAGE_SIZE;
		}

		return (void*)segment_break;
	}



	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING====
	//return (void*)-1 ;
	//panic("not implemented yet");
}

void* kmalloc(unsigned int size)
{
//	cprintf("Kmalloc@ with size = %d\n",size);

//	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
//	//refer to the project presentation and documentation for details
//	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
//
//	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
//
	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
//		cprintf("alloc_block_ff\n");
		return alloc_block_FF(size);
	}
	size = ROUNDUP(size, PAGE_SIZE); size /= PAGE_SIZE;

	uint32 last_page = hard_limit + PAGE_SIZE;
	uint32 cnt = 0;
	uint32 found = 0;

	for(uint32 cur_page = last_page; cur_page < KERNEL_HEAP_MAX; cur_page += PAGE_SIZE)
	{
		uint32* ptr_page_table = NULL;
		struct FrameInfo* frame = get_frame_info(ptr_page_directory,cur_page,&ptr_page_table);
		if(frame == NULL)
		{
			cnt++;
			if(cnt == 1)
				last_page = cur_page;
		}
		else
			cnt = 0;

		if(cnt == size)
		{
			found = 1;
			break;
		}
	}


	if(!found)
		return FreeRAM(size);


//	cprintf("new_block from kmalloc begin \n");
	struct PA_block *new_block = alloc_block_FF(16);
//	cprintf("new_block from kmalloc end \n");

	new_block->no_pages = size;
	new_block->va = last_page;

	LIST_INSERT_HEAD(&PA_LIST, new_block);

	for(uint32 cur_page = last_page, i = 0; i < size; i++, cur_page += PAGE_SIZE)
	{
		uint32* ptr_page_table;
		struct FrameInfo * frame = get_frame_info(ptr_page_directory,cur_page,&ptr_page_table);
		allocate_frame(&frame);
		map_frame(ptr_page_directory, frame, cur_page, PERM_WRITEABLE);
		frame->va = cur_page;
	}

	return (void*)last_page;
}

void kfree(void* virtual_address)
{
//	cprintf("Kfree@\n");
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//	panic("kfree() is not implemented yet...!!");

    if((unsigned int)virtual_address < KERNEL_HEAP_START && (unsigned int)virtual_address >= KERNEL_HEAP_MAX)
		panic("INVALID ADDRESS\n");


	if((uint32)virtual_address >= (uint32)memory_list.lh_first + sizeOfMetaData() && (uint32)virtual_address < (uint32)memory_list.lh_last + memory_list.lh_last->size)
	{
		free_block(virtual_address);
		return;
	}

	struct PA_block *element;
	LIST_FOREACH(element, &PA_LIST)
	{
		if((uint32)virtual_address == element->va)
		{
			for(int i = 0 ; i < element->no_pages; i++)
			{
				uint32 curVa = (uint32)virtual_address + i * PAGE_SIZE;
				uint32* ptr_page_table;
				struct FrameInfo* curFrame = get_frame_info(ptr_page_directory,curVa,&ptr_page_table);
				curFrame->va = 0;
				unmap_frame(ptr_page_directory,curVa);
			}
			element->no_pages = element->va = 0;
			LIST_REMOVE(&PA_LIST,element);
			free_block(element);
			return;
		}
	}
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{

	uint32 mask = (PAGE_SIZE - 1);
	uint32 offset = mask & physical_address;

	struct FrameInfo *ptr_frame_info = to_frame_info(physical_address);
    if((unsigned int)ptr_frame_info->va >= KERNEL_HEAP_START && (unsigned int)ptr_frame_info->va < KERNEL_HEAP_MAX && ptr_frame_info->references != 0)
		return (unsigned int)ptr_frame_info->va + offset;
//    cprintf("\n================%d\n\%d\n%d\n====================\n",KERNEL_HEAP_START,ptr_frame_info->va,KERNEL_HEAP_MAX);
	return 0;

	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	uint32 *ptr_page_table=NULL;
	get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);
	if (ptr_page_table != NULL && virtual_address >= KERNEL_HEAP_START && virtual_address < KERNEL_HEAP_MAX)
	{
		uint32 mask = (PAGE_SIZE - 1);
		uint32 offset = mask & virtual_address;

		//frame no 20 bit perms 12 bit
		uint32 entry = ptr_page_table[PTX(virtual_address)] & 0xFFFFF000;
		uint32 physical_address = entry + offset;

		return physical_address;
	}
	else
		return 0;


	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	//Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	//return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	if(virtual_address == NULL)
		return kmalloc(new_size);

	if(new_size == 0)
	{
		kfree(virtual_address);
		return NULL;
	}

	if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE)
		realloc_block_FF(virtual_address, new_size);

	struct PA_block* element;
	LIST_FOREACH(element, &PA_LIST)
		if(element->va == (uint32)virtual_address)
			break;
	uint32 cnt = element->no_pages;
	uint32 number_of_needed_pages = ROUNDUP(new_size, PAGE_SIZE);
	number_of_needed_pages /= PAGE_SIZE;

	if(number_of_needed_pages == element->no_pages)
		return (void*)element->va;

	if(number_of_needed_pages > element->no_pages)
	{

		number_of_needed_pages -= element->no_pages;

		uint32 temp = number_of_needed_pages;

		uint8 found = 0;
		for(uint32 cur_page = element->va + element->no_pages * PAGE_SIZE; cur_page < KERNEL_HEAP_MAX && number_of_needed_pages; cur_page += PAGE_SIZE, number_of_needed_pages--)
		{
			uint32* ptr_page_table;
			struct FrameInfo * frame = get_frame_info(ptr_page_directory,cur_page,&ptr_page_table);
			if(frame != NULL)
				break;
		}

		if(number_of_needed_pages != 0)
		{
			void* p = kmalloc(new_size);

			if(p != NULL)
			{
				for(uint32 adder = (uint32)virtual_address, new_addr = ((uint32)p),i = 0; i < cnt;i++, adder += PAGE_SIZE,new_addr += PAGE_SIZE)
				{
					for(uint32 j = adder, k = new_addr; j < adder + PAGE_SIZE; j++, k++)
					{
						uint8* p1 = (uint8*)(j);
						uint8* p2 = (uint8*)(k);
						*p2 = *p1;
					}
				}

				kfree(virtual_address);

				return p;
			}

		}

		number_of_needed_pages = temp;

		element->no_pages += number_of_needed_pages;

		for(uint32 cur_page = element->va + element->no_pages * PAGE_SIZE; number_of_needed_pages; cur_page += PAGE_SIZE,number_of_needed_pages--)
		{
			uint32* ptr_page_table;
			struct FrameInfo * frame = get_frame_info(ptr_page_directory,cur_page,&ptr_page_table);
			allocate_frame(&frame);
			map_frame(ptr_page_directory,frame,cur_page, PERM_WRITEABLE);
			frame->va = cur_page;
		}

		return (void*)element->va;
	}
	else
	{
		uint32 number_of_abandoned_pages = element->no_pages - number_of_needed_pages;

		element->no_pages -= number_of_abandoned_pages;

		for(uint32 cur_page = element->va + element->no_pages * PAGE_SIZE - PAGE_SIZE; number_of_abandoned_pages-- ; cur_page -= PAGE_SIZE)
		{
			uint32* ptr_page_table;
			struct FrameInfo * frame = get_frame_info(ptr_page_directory, cur_page, &ptr_page_table);
			unmap_frame(ptr_page_directory,cur_page);
			frame->va = 0;
		}

		return (void*)element->va;
	}
}

