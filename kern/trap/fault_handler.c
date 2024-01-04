/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"
#include "../mem/kheap.h"
//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
//		cprintf("\n%d\n",fault_va);
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));

#else
		int iWS = curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
		//cprintf("=============== LIST IN THE BEGINNNING : ================ \n");
		//env_page_ws_print(curenv);
		fault_va = ROUNDDOWN(fault_va,PAGE_SIZE);

		//cprintf("REPLACEMENT========================= WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details

		if(isPageReplacmentAlgorithmFIFO())
		{
			if(wsSize < (curenv->page_WS_max_size))
			{
//				cprintf("Placement\n");
				struct FrameInfo* ptr;
				allocate_frame(&ptr);
				map_frame(curenv->env_page_directory,ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
				int ret = pf_read_env_page(curenv, (void*)fault_va);
				if(ret == E_PAGE_NOT_EXIST_IN_PF)
				{
					if (!((USER_HEAP_START <= fault_va && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
					{
						sched_kill_env(curenv->env_id);
					}
				}
				struct WorkingSetElement* last = env_page_ws_list_create_element(curenv, fault_va);
				uint32* ptr_page_table;
				struct FrameInfo* frame = get_frame_info(curenv->env_page_directory, fault_va, &ptr_page_table);
				LIST_INSERT_TAIL(&(	curenv->page_WS_list), last);
				frame->element = last;
				if(curenv->page_WS_list.size == curenv->page_WS_max_size)
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			}
			else
			{
//				cprintf("Replacement\n");
				//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
				// Write your code here, remove the panic and write your code
				//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");

				struct FrameInfo* ptr;
				allocate_frame(&ptr);
				map_frame(curenv->env_page_directory,ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
				struct WorkingSetElement* last = curenv->page_last_WS_element;

				int perms = pt_get_page_permissions(curenv->env_page_directory, last->virtual_address);

				int ret = pf_read_env_page(curenv, (void*)fault_va);

				if(ret == E_PAGE_NOT_EXIST_IN_PF)
				{
					if (!((USER_HEAP_START <= fault_va && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
					{
	//					cprintf("killl");
						sched_kill_env(curenv->env_id);
					}
				}
				struct WorkingSetElement* page = env_page_ws_list_create_element(curenv, fault_va);

				struct WorkingSetElement* to_insert;

				if(curenv->page_last_WS_element->prev_next_info.le_prev == NULL)
					LIST_INSERT_HEAD(&(curenv->page_WS_list), page);
				else
				{
					to_insert = last->prev_next_info.le_prev;

					LIST_INSERT_AFTER(&(curenv->page_WS_list),to_insert , page);
				}

				uint32 *ptr_page_table;

				struct FrameInfo *frame = get_frame_info(curenv->env_page_directory, last->virtual_address, &ptr_page_table);

				if(perms & PERM_MODIFIED)
				{
					pf_update_env_page(curenv, last->virtual_address, frame);
				}

				LIST_REMOVE(&(curenv->page_WS_list), last);
				unmap_frame(curenv->env_page_directory,last->virtual_address);
				kfree(last);

				if(page->virtual_address == curenv->page_WS_list.lh_last->virtual_address)
				{
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else
				{
					curenv->page_last_WS_element = page->prev_next_info.le_next;
				}
			}
		}
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			// =========== IN LRU ALGORITHM ===========

			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER – LRU Placement
			 if((LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList))) < curenv->page_WS_max_size)
			 {
				 bool faulted_page_found = 0;
				 struct WorkingSetElement *ptr_element;
				 struct WorkingSetElement *ptr_element_found;

				 LIST_FOREACH(ptr_element , &(curenv->SecondList))
				 {
					if(fault_va == ptr_element->virtual_address)
					{
						faulted_page_found = 1;
						ptr_element_found = ptr_element;
						LIST_REMOVE(&(curenv->SecondList),ptr_element);
						break;
					}
				 }

				 if(faulted_page_found)
				 {
					 // cprintf("Already found page in Second list \n");
					 struct WorkingSetElement *ptr_element_to_remove_from_1 = curenv->ActiveList.lh_last;
					 LIST_REMOVE(&(curenv->ActiveList) , ptr_element_to_remove_from_1);
					 LIST_INSERT_HEAD(&(curenv->SecondList),ptr_element_to_remove_from_1);
					 LIST_INSERT_HEAD(&(curenv->ActiveList), ptr_element_found);
					 pt_set_page_permissions(curenv->env_page_directory ,(uint32)curenv->ActiveList.lh_first->virtual_address, PERM_PRESENT,0x000);

				 }

				 else
				 {
					struct FrameInfo* ptr;
					allocate_frame(&ptr);
					map_frame(curenv->env_page_directory, ptr ,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
					int ret = pf_read_env_page(curenv, (void*)fault_va);
					if(ret == E_PAGE_NOT_EXIST_IN_PF)
					{
						//cprintf("E_PAGE_NOT_EXIST_IN_PF \n");
						if (!((USER_HEAP_START <= fault_va && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
						{
							//cprintf("KILL HEREE \n");
							sched_kill_env(curenv->env_id);
						}
					}
					 if(curenv->ActiveList.size != curenv->ActiveListSize)
					 {
						 // =========== IN ACTIVE LIST ===========
						 struct WorkingSetElement* page = env_page_ws_list_create_element(curenv, fault_va);
						 LIST_INSERT_HEAD(&(curenv->ActiveList) , page );
					 }
					 else
					 {
						// =========== IN CASE SECOND LIST ============
						struct WorkingSetElement* page = env_page_ws_list_create_element(curenv, fault_va);
						struct WorkingSetElement *ptr_element_to_remove;
						ptr_element_to_remove = curenv->ActiveList.lh_last;
						struct WorkingSetElement *ptr_element_to_remove_from_1 = curenv->ActiveList.lh_last ;
						LIST_REMOVE(&(curenv->ActiveList),ptr_element_to_remove_from_1);
						LIST_INSERT_HEAD(&(curenv->SecondList) , ptr_element_to_remove);
						pt_set_page_permissions(curenv->env_page_directory ,(uint32)curenv->SecondList.lh_first->virtual_address,0x000,PERM_PRESENT);
						LIST_INSERT_HEAD(&(curenv->ActiveList), page);
					 }
				 }

			 } //TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - LRU Replacement

			 else
			 {
                 //	IN LRU REPLACEEMENTTTTTTTTT

				 struct WorkingSetElement *ptr_element;
				 struct WorkingSetElement *ptr_element_found_2 = 0;
				 bool faulted_page_found = 0 ;
				 LIST_FOREACH(ptr_element , &(curenv->SecondList))
				 {
					if(fault_va == ptr_element->virtual_address)
					{
						faulted_page_found = 1;
						ptr_element_found_2 = ptr_element;
						LIST_REMOVE(&(curenv->SecondList) , ptr_element);
						break;
					}
				 }

				 if(faulted_page_found)
				 {
					 //cprintf("Already found page in Second list \n");
					 //cprintf("after remove found page in second size : %d \n " , curenv->ActiveList.size);
					 //Overflow
					 //Get the tail of ActiveList and put in head SecondList and delete it from ActiveList
					 struct WorkingSetElement *ptr_tail_of_active = curenv->ActiveList.lh_last;
					 LIST_REMOVE(&(curenv->ActiveList) , ptr_tail_of_active);
					 //cprintf("after remove tail of active size : %d \n " , curenv->ActiveList.size);
					 LIST_INSERT_HEAD(&(curenv->SecondList) , ptr_tail_of_active );
					 //put head SecondList present = 0 (permissions_to_clear)
					 pt_set_page_permissions(curenv->env_page_directory ,(uint32)ptr_tail_of_active->virtual_address,0x000,PERM_PRESENT);
					 //put founded fault_va in head of ActiveList
					 LIST_INSERT_HEAD(&(curenv->ActiveList) , ptr_element_found_2);
					 //put head ActiveList present = 1 (permissions_to_set)
					 pt_set_page_permissions(curenv->env_page_directory , (uint32)curenv->ActiveList.lh_first->virtual_address , PERM_PRESENT,0x000);
				 }

				 else
				 {
					//NOT in Second list

					struct FrameInfo* ptr;
					allocate_frame(&ptr);
					map_frame(curenv->env_page_directory, ptr , fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
					int ret = pf_read_env_page(curenv, (void*)fault_va);
					if(ret == E_PAGE_NOT_EXIST_IN_PF)
					{
						if (!((USER_HEAP_START <= fault_va && fault_va < USER_HEAP_MAX) || (fault_va >= USTACKBOTTOM && fault_va < USTACKTOP)))
						{
							//cprintf("KILL HEREE \n");
							sched_kill_env(curenv->env_id);
						}
					}

					 struct WorkingSetElement* page = env_page_ws_list_create_element(curenv, fault_va);
					 //Put tail of SecondList (victim) on Disk if modified curenv->SecondList.lh_last
					 struct WorkingSetElement *ptr_tail_of_second = curenv->SecondList.lh_last;
					 uint32 va_victim = curenv->SecondList.lh_last->virtual_address;
					 int perms = pt_get_page_permissions(curenv->env_page_directory,va_victim );

					uint32 *ptr_page_table;
					struct FrameInfo* frame = get_frame_info(curenv->env_page_directory, va_victim, &ptr_page_table);
					 if((perms & PERM_MODIFIED) )  //Modified
					 {
						 pf_update_env_page(curenv,va_victim, frame);

					 }
					 //unmap_frame(curenv->env_page_directory,ptr_tail_of_second->virtual_address);
					 env_page_ws_invalidate(curenv , va_victim);

					struct WorkingSetElement *ptr_element_to_remove;
					ptr_element_to_remove = curenv->ActiveList.lh_last;
					LIST_REMOVE(&(curenv->ActiveList),ptr_element_to_remove);
					LIST_INSERT_HEAD(&(curenv->SecondList) , ptr_element_to_remove);
					pt_set_page_permissions(curenv->env_page_directory ,(uint32)curenv->SecondList.lh_first->virtual_address,0x000,PERM_PRESENT);
					LIST_INSERT_HEAD(&(curenv->ActiveList), page);
				 }

			 }

			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");
			 //cprintf("================== END OF LRU , LISTS AFTER :=========================== \n");
			 //env_page_ws_print(curenv);
			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
}
void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}
