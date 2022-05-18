#include <inc/stdio.h>
#include <kern/priority_manager.h>
#include <inc/assert.h>
#include <kern/helpers.h>
#include <kern/user_environment.h>
#include <kern/memory_manager.h>
#include <kern/trap.h>

void clearEntriesInWorkingSet();
void addWorkingSetEntries(struct Env* env,uint32 size);


void set_program_priority(struct Env* env, int priority) {
	//TODO: [PROJECT 2022 - BONUS4] Change WS Size according to Program Priority�

	env->priority = priority;

	if (priority == PRIORITY_LOW) {
		clearEntriesInWorkingSet(env,((env->page_WS_max_size/2)+(env->page_WS_max_size%2)));
	}
	else if(priority == PRIORITY_BELOWNORMAL){
		if(env_page_ws_get_size(env) <= (env->page_WS_max_size/2)){
			clearEntriesInWorkingSet(env,((env->page_WS_max_size/2)+(env->page_WS_max_size%2)));
		}
	}
	else if(priority == PRIORITY_ABOVENORMAL && env->wsMaxSizeDoubled != 1){
		if(env_page_ws_get_size(env) == env->page_WS_max_size){
			addWorkingSetEntries(env,env->page_WS_max_size);
			env->page_WS_max_size*=2;
			env->wsMaxSizeDoubled = 1;
		}
	}
	else if(priority == PRIORITY_HIGH){
		if(env_page_ws_get_size(env) == env->page_WS_max_size && env->page_WS_max_size*2 <= 100){
			addWorkingSetEntries(env,env->page_WS_max_size);
			env->page_WS_max_size *= 2;
			env->wsMaxSizeDoubled = 1;
		}
	}
}

void clearEntriesInWorkingSet(struct Env* env, uint32 size) {

	for(uint32 entry = 0; entry < size; entry++,env->page_WS_max_size--){
		uint32 deletedEntryIdx = modifiedClock(env);
		for(uint32 entry = deletedEntryIdx; entry < (env->page_WS_max_size - 1); entry++){
			env->ptr_pageWorkingSet[entry].virtual_address = env->ptr_pageWorkingSet[entry+1].virtual_address;
			env->ptr_pageWorkingSet[entry].empty = env->ptr_pageWorkingSet[entry+1].empty;
			env->ptr_pageWorkingSet[entry].time_stamp = env->ptr_pageWorkingSet[entry+1].time_stamp;
		}
		env->ptr_pageWorkingSet[env->page_WS_max_size - 1].virtual_address = 0;
		env->ptr_pageWorkingSet[env->page_WS_max_size - 1].empty = 1;
		env->ptr_pageWorkingSet[env->page_WS_max_size - 1].time_stamp = 0;
	}
}

void addWorkingSetEntries(struct Env* env,uint32 size){

	for (int entry = env->page_WS_max_size; entry < (env->page_WS_max_size + size); entry++) {
		env->ptr_pageWorkingSet[entry].virtual_address = 0;
		env->ptr_pageWorkingSet[entry].empty = 1;
		env->ptr_pageWorkingSet[entry].time_stamp = 0;
	}

}

