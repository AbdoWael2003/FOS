#include <inc/lib.h>


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
int FirstTimeFlag = 1;




void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
//	cprintf("malloc@ with size = %d\n",size);
	//	cprintf("%d<<<<<<<<<<<<<<<<<<\n",NUM_OF_KHEAP_PAGES);
	//	cprintf("%d<<@<<\n",FREE_USER_PAGES.size);
	//	cprintf("%d<<@<<\n",FREE_USER_PAGES.lh_first->no_pages);
		//==============================================================
		//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();

	uint32 hard_limit = (uint32)sys_call_hard_limit_user();
//
//	cprintf("%d<<@<<\n",FREE_USER_PAGES.size);
//	cprintf("%d<<@<<\n",FREE_USER_PAGES.lh_first->no_pages);

	if (size == 0) return NULL;

	if(size <= DYN_ALLOC_MAX_BLOCK_SIZE)
	{
//		cprintf("alloc_block_ff\n");
		return alloc_block_FF(size);
	}
//	cprintf("malloc_PAGE_ALLOCATOR\n");

	uint32 original_size = size;

	size = ROUNDUP(size, PAGE_SIZE);
	size /= PAGE_SIZE;

	uint32 last_page = hard_limit + PAGE_SIZE;
	uint32 cnt = 0;
	uint32 found = 0;
	uint32 block_start_index;
	uint32 start_index = ((hard_limit - USER_HEAP_START) / PAGE_SIZE) + 1;

	for(uint32 cur_page = last_page; cur_page < USER_HEAP_MAX; cur_page += PAGE_SIZE,start_index++)
	{
		if (!is_marked[start_index])
		{
			cnt++;
			if(cnt == 1)
			{
				last_page = cur_page;
				block_start_index = start_index;
			}
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
		return NULL;

	struct PA_block *new_block = alloc_block_FF(16);

	new_block->no_pages = size;
	new_block->va = last_page;

	LIST_INSERT_HEAD(&USER_PA_LIST, new_block);

	for(int i = block_start_index; i < block_start_index + size; i++)
		is_marked[i] = 1;


	sys_allocate_user_mem(last_page,original_size);
//	cprintf("the return is %d\n",last_page);
	return (void*)last_page;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//	panic("malloc() is not implemented yet...!!");
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
//	cprintf("free\n");


	if((unsigned int)virtual_address < USER_HEAP_START && (unsigned int)virtual_address >= USER_HEAP_MAX)
		panic("INVALID ADDRESS\n");

	uint32 hard_limit = (uint32)sys_call_hard_limit_user();
	if((uint32)virtual_address < hard_limit)
	{
		free_block(virtual_address);
		return;
	}

	struct PA_block *element;
	LIST_FOREACH(element, &USER_PA_LIST)
	{
		if((uint32)virtual_address == element->va)
		{
			int start = ((uint32)virtual_address - USER_HEAP_START) / PAGE_SIZE;
			for(int i = start; i < start + element->no_pages; i++)
				is_marked[i] = 0;

			sys_free_user_mem((uint32)virtual_address,element->no_pages * PAGE_SIZE);
			LIST_REMOVE(&USER_PA_LIST,element);

			break;
		}
	}
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}
