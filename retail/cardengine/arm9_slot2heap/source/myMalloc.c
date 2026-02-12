/*
	Code by Tharika Madurapperuma, with a fix by an anonymous 2024 user
	Support for Slot-2 RAM expansion by Rocket Robz

	Sources:
	https://tharikasblogs.blogspot.com/p/how-to-write-your-own-malloc-and-free.html
	https://tharikasblogs.blogspot.com/p/include-include-include-mymalloc.html
*/

#include <nds/ndstypes.h>
#include <stdio.h>
#include <stddef.h>

extern u32 memory;
extern u32 memorySize;

struct block{
	size_t size;
	int free;
	struct block *next; 
};

struct block *freeList=NULL;
void initialize() {
	freeList=(void*)memory;

	freeList->size=memorySize-sizeof(struct block);
	freeList->free=1;
	freeList->next=NULL;
}

void split(struct block *fitting_slot,size_t size) {
	struct block *new=(void*)((void*)fitting_slot+size+sizeof(struct block));
	new->size=(fitting_slot->size)-size-sizeof(struct block);
	new->free=1;
	new->next=fitting_slot->next;
	fitting_slot->size=size;
	fitting_slot->free=0;
	fitting_slot->next=new;
}


void *MyMalloc(size_t noOfBytes) {
	if ((noOfBytes % 4) != 0) {
		// Word-align
		noOfBytes /= 4;
		noOfBytes *= 4;
	}
	struct block *curr,*prev;
	void *result;
	if (!freeList) {
		initialize();
		// printf("Memory initialized\n");
	}
	curr=freeList;
	while ((((curr->size) < noOfBytes) || ((curr->free) == 0)) && (curr->next != NULL)) {
		prev=curr;
		curr=curr->next;
		// printf("One block checked\n");
	}
	if ((curr->size)==noOfBytes) {
		curr->free=0;
		result=(void*)(++curr);
		// printf("Exact fitting block allocated\n");
	} else if ((curr->size) >= (noOfBytes+sizeof(struct block))) {
		split(curr,noOfBytes);
		result=(void*)(++curr);
		// printf("Fitting block allocated with a split\n");
	} else {
		result=NULL;
		// printf("Sorry. No sufficient memory to allocate\n");
	}
	return result;
}

void merge() {
	struct block *curr,*prev;
	curr=freeList;
	while ((curr->next) != NULL) {
		if ((curr->free) && (curr->next->free)) {
			curr->size+=(curr->next->size)+sizeof(struct block);
			curr->next=curr->next->next;
		} else {
			prev=curr;
			curr=curr->next;
		}
	}
}

void MyFree(void* ptr) {
	if (((void*)memory<=ptr) && (ptr<=(void*)(memory+memorySize))) {
		struct block* curr=ptr;
		--curr;
		curr->free=1;
		merge();
	}
	// else printf("Please provide a valid pointer allocated by MyMalloc\n");
}