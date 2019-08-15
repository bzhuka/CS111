#include "SortedList.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
	if (element == NULL)
		return;

	SortedListElement_t *pIter = list;
	SortedListElement_t *pNext = list->next;

	while(pNext != NULL) {
		if((*(pNext->key)) >= (*(element->key))) {
			break;
		}
		pIter = pNext;
		pNext = pNext->next;
	}

	if (opt_yield & INSERT_YIELD) {
		sched_yield();
	}
	if(pNext == NULL) {
		element->prev = pIter;
		element->next = NULL;
		pIter->next = element;
	} else { // Every other case
		//pIter points to the item right before it should be inserted
		element->prev = pIter;
		element->next = pNext;
		pIter->next = element;
		pNext->prev = element;
	}
}

int SortedList_delete(SortedListElement_t *element) {
	if (element == NULL)
		return 1;

	SortedListElement_t *pNext = element->next;
	SortedListElement_t *pPrev = element->prev;

	//DO WE HAVE TO CHECK IF IT IS LIST HEAD
	//If it is the last node
	if (pNext == NULL) {
		if (pPrev->next != element) {
			return 1;
		}
	} else if (pNext->prev != element || pPrev->next != element) {
		return 1;
	}

	if (pNext != NULL) {
		pNext->prev = pPrev;
	}
	//NOT SURE IF THIS BELONGS HERE?
	if (opt_yield & DELETE_YIELD) {
		sched_yield();
	}
	pPrev->next = pNext;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	SortedListElement_t *pIter = list->next;

	while( pIter != NULL) {
		if((*(pIter->key)) == (*key)) {
			if (opt_yield & LOOKUP_YIELD) {
				sched_yield();
			}
			return pIter;
		}
		pIter = pIter->next;
	}

	if (opt_yield & LOOKUP_YIELD) {
		sched_yield();
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) {
	int counter = 1;
	SortedListElement_t *pIter = list->next;

	// If it is an empty list
	if(list->next == NULL) {
		return 0;
	}

	//Checks all but last
	while(pIter->next != NULL) {
		if (pIter->next->prev != pIter || pIter->prev->next != pIter) {
			return -1;
		}
		pIter = pIter->next;
		counter++;
	}

	//Checks last
	if (pIter->prev->next != pIter) {
		return -1;
	}

	if (opt_yield & LOOKUP_YIELD) {
		sched_yield();
	}

	return counter;
}
