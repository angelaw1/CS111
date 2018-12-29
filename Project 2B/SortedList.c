// NAME: Angela Wu
// EMAIL: angelawu.123456789@gmail.com
// ID: 604763501

#include "SortedList.h"
#include <stdio.h>
#include <string.h>
#include <sched.h>

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {

	if (list == NULL || element == NULL || element->key == NULL) {
		return;
	}
	SortedListElement_t *temp = list;
	while (temp->next != list && temp->next != list &&
		// *(temp->next->key) <= *(element->key)) {
		strcmp(temp->next->key, element->key) <= 0) {
		temp = temp->next;
	}
	element->next = temp->next;
	element->prev = temp;
	if (opt_yield & INSERT_YIELD)
		sched_yield();
	temp->next->prev = element;
	temp->next = element;

}

int SortedList_delete(SortedListElement_t *element) {
	if (element == NULL || element->next->prev != element || element->prev->next != element) {
		fprintf(stderr, "pointers not pointing to correct element\n");
		return 1;
	}
	element->prev->next = element->next;
	if (opt_yield & DELETE_YIELD)
		sched_yield();
	element->next->prev = element->prev;
	return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
	if (list == NULL || key == NULL) {
		return NULL;
	}
	SortedListElement_t *temp = list->next;
	while (temp != list && strcmp(temp->key, key) <= 0) {
		if (strcmp(temp->key, key) == 0) {
			return temp;
		}
		if (opt_yield & LOOKUP_YIELD)
			sched_yield();
		temp = temp->next;
	}
	return NULL;
}

int SortedList_length(SortedList_t *list) {
	int count = 0;
	if (list == NULL) {
		return -1;
	}
	SortedListElement_t *temp = list->next;
	while (temp != list) {
		if (temp->next->prev != temp || temp->prev->next != temp) {
			fprintf(stderr, "pointers don't point to correct element\n");
			return -1;
		}
		count++;
		if (opt_yield & LOOKUP_YIELD)
			sched_yield();
		temp = temp->next;
	}
	return count;
}
