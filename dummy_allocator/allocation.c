#include <unistd.h>
#include <string.h>
//#include <sys/syscall.h>
#include "allocation.h"

struct memoryInfo {
	void* ptr;
	size_t size;
	struct memoryInfo* nextInfo;
	int isFree;
};
static struct memoryInfo* memInfo = NULL;
static struct memoryInfo* memInfoEnd = NULL;

size_t alignment(size_t oldSize);
/*
void out() {
	struct memoryInfo* iterPtr = memInfo;
	for (;iterPtr != NULL; iterPtr = iterPtr->nextInfo) {
		printf("%ld isFree = %d\n", iterPtr->size, iterPtr->isFree);
	}
}*/


void* my_malloc(size_t size)
{
	//ADD PROTECT
	
	
	//First malloc using;
	int firstAlloc = 0;
	if (memInfo == NULL) {
		//memInfo = syscall(SYS_brk, sizeof(struct memoryInfo)); 
		memInfo = sbrk(sizeof(struct memoryInfo));
		memInfoEnd = memInfo;
		firstAlloc = 1;
	}
	
	void* newPtr = NULL; //Ptr for return
	
	//Fine free memory in list
	struct memoryInfo* iterPtr = memInfo;

	struct memoryInfo* reallocPtr = NULL;
	
	for (; iterPtr != NULL; iterPtr = iterPtr->nextInfo) {
		if (iterPtr->size >= size && iterPtr->isFree == 1) {
			reallocPtr = iterPtr;
			break;
		}
	}
	/*if (iterPtr->nextInfo != NULL) {
		do {
//		for (;iterPtr != NULL;iterPtr = iterPtr->nextInfo) {
			printf("down here");
			if (iterPtr->size >= size && iterPtr->isFree == 1) {
				reallocPtr = iterPtr;
				break;
			}
			iterPtr = iterPtr->nextInfo;
		} while (iterPtr->nextInfo != NULL); 
	} else if (iterPtr->size >= size && iterPtr->isFree == 1) {
		reallocPtr = iterPtr;
	}*/
	
	
	if (reallocPtr != NULL) {
		// If THERE IS ENOUGH MEMORY IN LIST
	
		newPtr = reallocPtr->ptr;// HERE NEW PTR IS READY FOR RETURN!
		
		size_t tmpSize = reallocPtr->size - size;
		reallocPtr->isFree = 0;
		reallocPtr->size = size; //Changed info
		
		if (tmpSize !=0) {
			struct memoryInfo* infoPtr = NULL;
			
			infoPtr = sbrk(sizeof(struct memoryInfo));
			
			
			infoPtr->ptr = reallocPtr->ptr + size;
			infoPtr->nextInfo = NULL;
			infoPtr->size = tmpSize;
			infoPtr->isFree = 1;
			//Find last list value
			//iterPtr = memInfo;
	/*		if (memInfo->nextInfo != NULL) {
				while (iterPtr->nextInfo != NULL) {
					iterPtr = memInfo->nextInfo;
				}
			} 
			iterPtr->nextInfo = infoPtr;
		*/ 	memInfoEnd->nextInfo = infoPtr;
			memInfoEnd = infoPtr;
		}
	} else {
	
		// IF THERE IS NO ENOUGH MEMORY IN LIST
		// Allocate memory
		size_t _size = alignment(size);
		newPtr = sbrk(_size);
		
		if (firstAlloc == 1) {
			memInfo->ptr = newPtr;
			memInfo->size = _size;
			memInfo->nextInfo = NULL;
			memInfo->isFree = 0;
			return newPtr; // When first alloc
		}
		
		//fill info list
		struct memoryInfo* infoPtr = NULL;
		infoPtr = sbrk(sizeof(struct memoryInfo));
		
		infoPtr->ptr = newPtr;
		infoPtr->size = size;
		infoPtr->nextInfo = NULL;
		infoPtr->isFree = 0;
		
		//Find last list value
		/*iterPtr = memInfo;
		if (memInfo->nextInfo != NULL) {
			while (iterPtr->nextInfo != NULL) {
				iterPtr = memInfo->nextInfo;
			}
		} 
		
		iterPtr->nextInfo = infoPtr;
		*/ 
		memInfoEnd->nextInfo = infoPtr;
		memInfoEnd = infoPtr;
		return newPtr;
	}
		
	return newPtr; 
}

void* my_calloc(size_t size) 
{
	void* ptr = NULL;
	ptr = my_malloc(size);
	memset(ptr, 0, size);
	
	return ptr;
}

void my_free(void* ptr)
{
		
	if (ptr == NULL) return;
	if (memInfo == NULL) return;
	// how to Protect of not heap memory??
	struct memoryInfo* iterPtr = memInfo;
	/*
	if (iterPtr->nextInfo == NULL) {
		if (iterPtr->ptr == ptr) {
			iterPtr->isFree = 1;
			return;
		} else 
			return;
	}*/
	//do {
	for (;iterPtr != NULL; iterPtr = iterPtr->nextInfo) {
		if (iterPtr->ptr == ptr) {
			iterPtr->isFree = 1;
			return;
		}		
		//iterPtr = iterPtr->nextInfo;
	} //while (iterPtr->nextInfo != NULL);
}


void* my_realloc(void* ptr, size_t size) 
{
	if (ptr == NULL) return NULL;
	if (memInfo == NULL) return NULL;
	void* newPtr = NULL;
	
	struct memoryInfo* iterPtr = memInfo;
	struct memoryInfo* oldReallocPtr = NULL;
	
	for (; iterPtr != NULL; iterPtr = iterPtr->nextInfo) {
		if (iterPtr->ptr == ptr) {
			oldReallocPtr = iterPtr;
			break;
		}
	}
	
	if (oldReallocPtr != NULL) {
		struct memoryInfo* newReallocPtr = NULL;
		
		for (; iterPtr != NULL; iterPtr = iterPtr->nextInfo) {
			if (iterPtr->size >= size && iterPtr->isFree == 1) {
				newReallocPtr = iterPtr;
				break;
			}
		}
		if (newReallocPtr != NULL) {
			
			newPtr = newReallocPtr->ptr;
			memset(newPtr, 0, size);
			strncpy(newPtr,oldReallocPtr->ptr, oldReallocPtr->size); //newPtr is ready for using
				
			oldReallocPtr->isFree = 1; // Free old Memory		
			
			size_t tmpSize = newReallocPtr->size - size;
			newReallocPtr->isFree = 0;
			newReallocPtr->size = size;
			
			if (tmpSize != 0) {
				struct memoryInfo* infoPtr = NULL;
				
				infoPtr = sbrk(sizeof(struct memoryInfo));
				
				infoPtr->ptr = newReallocPtr->ptr + size;
				infoPtr->nextInfo = NULL;
				infoPtr->size = tmpSize;
				infoPtr->isFree = 1;
				//Find last list value
				/*iterPtr = memInfo;
				if (memInfo->nextInfo != NULL) {
					while (iterPtr->nextInfo != NULL) {
						iterPtr = memInfo->nextInfo;
					}
				} 
				iterPtr->nextInfo = infoPtr;
				*/
				memInfoEnd->nextInfo = infoPtr;
			} 
			
		} else { // ELSE FOR if (newReallocPtr == NULL);
			struct memoryInfo* newAlloc;
			newAlloc = sbrk(sizeof(struct memoryInfo));
			size_t _size = alignment(size);
			newPtr = sbrk(_size);
			memset(newPtr, 0, _size);
			memcpy(newPtr, oldReallocPtr->ptr, oldReallocPtr->size);
			
			oldReallocPtr->isFree = 1;
			
			newAlloc->size = _size;
			newAlloc->nextInfo = NULL;
			newAlloc->ptr = newPtr;
			newAlloc->isFree = 0;
			
			
	/*		iterPtr = memInfo;
			for(; iterPtr->nextInfo != NULL; iterPtr = iterPtr->nextInfo) {
			}
			iterPtr->nextInfo = newAlloc;
			*/
			memInfoEnd->nextInfo = newAlloc;
			memInfoEnd = newAlloc;		
		}
		
				
	} else { // ELSE FOR if (olderReallocPtr != NULL) 
		return NULL;
	}
	return newPtr;
}


size_t alignment(size_t oldSize)
{
	if (oldSize % 8 == 0) {
		return oldSize;
	} else {
		size_t tmp = 0;
		tmp = oldSize / 8 * 8 + 8;
		return tmp;
	}
}
