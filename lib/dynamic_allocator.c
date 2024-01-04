/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
#include <inc/lib.h>

uint8 is_marked[NUM_OF_UHEAP_PAGES] = {};

//============================= OUR HELPER FUNCTIONS==============================

void CopyContent(void* old_va, void* new_va)
{
	for(uint32 addr =  (uint32)old_va, new_addr = (uint32)new_va; addr < (uint32)old_va + sizeOfMetaData() + ((struct BlockMetaData*)((uint32)old_va - sizeOfMetaData()))->size - sizeOfMetaData() ;addr++, new_addr++)
	{
		uint8* p1 = (uint8*)addr;
		uint8* p2 = (uint8*)new_addr;
		*p2 = *p1;
	}
}


int UpdateBlockAllocator(uint32 old_break, uint32 new_break)
{
//	cprintf("UpdateBlockAllocator old_break - new_break = %d\n",old_break - new_break);
	if(new_break == old_break)
		return 0;
	else
	{
		if(new_break - old_break > sizeOfMetaData())
		{
			struct BlockMetaData* new_block;
			if(memory_list.size == 0)
				new_block = (struct BlockMetaData*) memory_list.lh_first;
			else
				new_block = (struct BlockMetaData*) old_break;

			new_block->is_free = 1;
			new_block->size = new_break - old_break;
			LIST_INSERT_TAIL(&memory_list,new_block);
			return 1;
		}
		return 0;
	}
}

void split_block(struct BlockMetaData* blk, uint32 size)
{
	if(blk->size - sizeOfMetaData() > size && blk->size - size - sizeOfMetaData() > sizeOfMetaData())
	{

		struct BlockMetaData *new_block = ((void*)blk + size + sizeOfMetaData());
		new_block->size = blk->size - size - sizeOfMetaData();

		blk->is_free = 0;
		blk->size = size + sizeOfMetaData();

		LIST_INSERT_AFTER(&memory_list, blk, new_block);
		free_block(new_block + 1);
	}
}

int get_index(void* va)
{
	struct BlockMetaData* blk ;
	int i = 0;
	LIST_FOREACH(blk, &memory_list)
	{
		if((uint32)va - sizeOfMetaData() == (uint32)blk)
			return i;
		i++;
	}
	return -1;
}

//================================================================================

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	uint32 i = 0;
	LIST_FOREACH(blk, &list)
	{
		cprintf("(index = %u, size: %d, isFree: %d, Address: %p)\n",i++, blk->size, blk->is_free,blk) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	if (initSizeOfAllocatedSpace < 0)
		return;

	struct BlockMetaData *newBlkMetaData = ((struct BlockMetaData *)daStart);
	newBlkMetaData->size = initSizeOfAllocatedSpace;
	newBlkMetaData->is_free = 1;
	newBlkMetaData->prev_next_info.le_prev = NULL;
	newBlkMetaData->prev_next_info.le_next = NULL;
	LIST_INSERT_HEAD(&memory_list, newBlkMetaData);
}


//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void* SearchFF(uint32 size) // start of the current metadata
{
	struct BlockMetaData *blk;
	LIST_FOREACH(blk, &memory_list)
	{
		if (blk->size - sizeOfMetaData() >= size && blk->is_free)
		{
			split_block(blk,size);
			blk->is_free = 0;
			return (void*)blk + sizeOfMetaData();
		}
	}
	return NULL;
}

void *alloc_block_FF(uint32 size)
{
//	cprintf("alloc_block_ff with size = %u  total size = %u\n",size, size + sizeOfMetaData());
	if (size == 0)
		return NULL;

	if (!is_initialized)
	{
//		cprintf("DR.\n");
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	void* p = SearchFF(size);

	if(p != NULL)
	{
		return p;
	}

	if((uint32)sbrk(size + sizeOfMetaData()) == -1)
		return NULL;

	int ret = UpdateBlockAllocator((uint32)memory_list.lh_last + memory_list.lh_last->size,(uint32)sbrk(0));
	if(ret)
	{
		struct BlockMetaData* allocated = memory_list.lh_last;
		memory_list.lh_last->is_free = 0;
		split_block(memory_list.lh_last,size);
		return allocated + 1;
	}
	return NULL;
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//panic("alloc_block_FF is not implemented yet");
}
//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void* SearchBF(uint32 size)
{
	struct BlockMetaData *blk;

	struct BlockMetaData *best = NULL;
	uint32 mini_diff = -1;

	LIST_FOREACH(blk, &memory_list)
	{
		if (blk->size - sizeOfMetaData() >= size && blk->is_free && (best == NULL || blk->size - sizeOfMetaData() - size < mini_diff))
		{
			mini_diff = blk->size - sizeOfMetaData() - size;
			best = blk;
		}
	}

	if(best == NULL)
		return NULL;

	split_block(best, size);

	best->is_free = 0;
	return (void*)best + sizeOfMetaData();
}

void *alloc_block_BF(uint32 size)
{
	if (size == 0)
		return NULL;

	void* p = SearchBF(size);

	if(p != NULL)
		return p;

	if((uint32)sbrk(size) == -1)
		return NULL;


	int ret = UpdateBlockAllocator((uint32)memory_list.lh_last + memory_list.lh_last->size,(uint32)sbrk(0));
	if(ret)
	{
		struct BlockMetaData* allocated = memory_list.lh_last;
		memory_list.lh_last->is_free = 0;
		split_block(memory_list.lh_last,size);
		return allocated + 1;
	}
	return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}
//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	uint8 bf = 0,af = 0;

//	cprintf("free_block with va = %p with size = %u with index = %d\n",va,((struct BlockMetaData*)(va - 16))->size, get_index(va));
	if (va == NULL)
	{
		return;
	}

	int found = 0;
	struct BlockMetaData* blk;
	LIST_FOREACH(blk, &memory_list)
	{
		if(va == (blk + 1))
		{
			found = 1;
			break;
		}
	}
	if(!found)
		return;
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//	panic("free_block is not implemented yet");
	struct BlockMetaData *curBlkMetaData = (va - sizeOfMetaData()) ;
	uint32 size_of_cur_block = curBlkMetaData->size;


	struct BlockMetaData* prev_element = curBlkMetaData->prev_next_info.le_prev;
	struct BlockMetaData* next_element = curBlkMetaData->prev_next_info.le_next;

	if(prev_element != NULL && prev_element->is_free == 1)
	{
		bf = 1;
		size_of_cur_block += prev_element->size;
		memory_list.size--;
		if(next_element != NULL)
		{
			next_element->prev_next_info.le_prev = prev_element;
			prev_element->prev_next_info.le_next = next_element;
		}
		else
		{
			prev_element->prev_next_info.le_next = NULL;
			memory_list.lh_last = prev_element;
		}

		curBlkMetaData->is_free = 0;
		curBlkMetaData->size = 0;
		curBlkMetaData->prev_next_info.le_next = NULL;
		curBlkMetaData->prev_next_info.le_prev = NULL;

		curBlkMetaData = prev_element;
	}
	else if(prev_element == NULL)
		memory_list.lh_first = curBlkMetaData;
	if(next_element != NULL && next_element->is_free == 1)
	{
		af = 1;
		size_of_cur_block += next_element->size;
		memory_list.size--;
		if(next_element->prev_next_info.le_next != NULL)
		{
			curBlkMetaData->prev_next_info.le_next = next_element->prev_next_info.le_next;
			next_element->prev_next_info.le_next->prev_next_info.le_prev = curBlkMetaData;
		}
		else
		{	curBlkMetaData->prev_next_info.le_next = NULL;
			memory_list.lh_last = curBlkMetaData;
		}

		next_element->is_free = 0;
		next_element->size = 0;
		next_element->prev_next_info.le_next = NULL;
		next_element->prev_next_info.le_prev = NULL;
	}
	else if(next_element == NULL)
			memory_list.lh_last = curBlkMetaData;


	curBlkMetaData->is_free = 1;
	curBlkMetaData->size = size_of_cur_block;


//	cprintf("free_block new_block with address = %p with index = %d with size = %u concatinated with before = %u, with after = %u \n",curBlkMetaData + 1,get_index(va), curBlkMetaData->size,bf,af);

}






//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{

//	cprintf("realloc_block_FF is_free = %u\n",is_free_block(va));

	uint32 final_size = new_size + sizeOfMetaData();
	if (new_size < 0)
	{
		return NULL;
	}
	if (va == NULL && new_size == 0)
	{
		return alloc_block_FF(0);
	}
	if (va == NULL && new_size != 0)
	{
		return alloc_block_FF(new_size);
	}
	if (va != NULL && new_size == 0)
	{
		free_block(va);
		return NULL;
	}

	int found = 0;
	struct BlockMetaData* blk;
	LIST_FOREACH(blk, &memory_list)
	{
		if(va == (blk + 1))
		{
			found = 1;
			break;
		}
	}
	if(!found)
		return NULL;

	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1);
	uint32 cur_blk_size = curBlkMetaData->size;

	if(final_size == curBlkMetaData->size)
	{
		curBlkMetaData->is_free = 0;
		return va;
	}

//	print_blocks_list(memory_list);

	if(final_size > curBlkMetaData->size)
	{
		int required_size = final_size - cur_blk_size;

		struct BlockMetaData *next_blk = curBlkMetaData->prev_next_info.le_next;
		while(next_blk != NULL && next_blk->is_free && required_size > 0)
		{
			required_size -= next_blk->size;
			next_blk = next_blk->prev_next_info.le_next;
		}
		if(required_size > 0)
		{

			void* p = SearchFF(new_size);

			if(p != NULL)
			{
				CopyContent(va,p);
				free_block(va);
				return p;
			}

			if((uint32)sbrk(new_size + sizeOfMetaData()) == -1)
				return va;

			int ret = UpdateBlockAllocator((uint32)memory_list.lh_last + memory_list.lh_last->size,(uint32)sbrk(0));
			if(ret)
			{
				struct BlockMetaData* allocated = memory_list.lh_last;
				CopyContent(va,allocated + 1);
				memory_list.lh_last->is_free = 0;
				split_block(memory_list.lh_last,new_size);
				return allocated + 1;
			}
			return va;

		}
		next_blk = curBlkMetaData->prev_next_info.le_next;
		required_size = final_size - cur_blk_size;
		while(next_blk != NULL && next_blk->is_free && required_size > 0)
		{
			if(next_blk->size == required_size)
			{
//				cprintf("next_blk->size == required_size run : %d",cnt);
				curBlkMetaData->size += next_blk->size;
				required_size = 0;
				next_blk->size = next_blk->is_free = 0;
				LIST_REMOVE(&memory_list,next_blk);
			}
			else if(next_blk->size > required_size)
			{
//				cprintf("\n next_blk->size > required_size ");

				if(next_blk->size - required_size  > sizeOfMetaData())
				{
//					cprintf("with spliting\n");

					curBlkMetaData->size += required_size;

					struct BlockMetaData *new_block = (void*)next_blk + required_size;

					new_block->size = next_blk->size - required_size;

					new_block->is_free = 1;


					next_blk->size = next_blk->is_free = 0;

					LIST_REMOVE(&memory_list,next_blk);

					LIST_INSERT_AFTER(&memory_list,curBlkMetaData,new_block);

				}
				else
				{
//					cprintf("without spliting\n");
					curBlkMetaData->is_free = 0;
					curBlkMetaData->size += next_blk->size;
					next_blk->size = next_blk->is_free = 0;
					LIST_REMOVE(&memory_list,next_blk);
				}
				required_size = 0;
			}
			else
			{
//				cprintf("next_blk->size < required_size run : %d iteration %d",cnt,cnt2++);

				curBlkMetaData->size += next_blk->size;
				next_blk->size = next_blk->is_free = 0;
				required_size -= next_blk->size;
				struct BlockMetaData *next = next_blk->prev_next_info.le_next;
				LIST_REMOVE(&memory_list,next_blk);
				next_blk = next;
				continue;
			}
			next_blk = next_blk->prev_next_info.le_next;
		}
		curBlkMetaData->is_free = 0;

		return va;
	}
	else
	{
//		cprintf("\n =================================================== \n");
		split_block(curBlkMetaData,new_size);
		return va;
	}

	return va;
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");
}
