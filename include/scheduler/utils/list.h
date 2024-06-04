/**
 * Intrusive list implementation
 */

#include <stddef.h>
#include <memory.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#ifndef SCHED_LIST_H
#define SCHED_LIST_H

// types
typedef struct IntrusiveNode IntrusiveNode;
typedef struct IntrusiveList IntrusiveList;
typedef struct IntrusiveList IntrusiveLinkedList;

// interface

IntrusiveList* IntrusiveListNew();

void IntrusiveListPushBack(IntrusiveList* self, void* node);

void IntrusiveListPushFront(IntrusiveList* self, void* node);

void* IntrusiveListPopFront(IntrusiveList* self);

size_t IntrusiveListSize(IntrusiveList* self);

bool IntrusiveListIsEmpty(IntrusiveList* self);

void IntrusiveListConcat(IntrusiveList* self, IntrusiveList* other);

// definitions

/**
 * Place as first member to make it intrusive
 */
struct IntrusiveNode {
    IntrusiveNode* next;
    IntrusiveNode* prev;
};

struct IntrusiveList {
    IntrusiveNode* head;
    IntrusiveNode* tail;
    size_t size;
};

/* Intrusive node */

bool IntrusiveNodeIsLinked(IntrusiveNode* node);

void IntrusiveNodeUnlink(IntrusiveNode* node);

/* Intrusive list */

IntrusiveList* IntrusiveListNew();

void IntrusiveListDelete(IntrusiveList* self);

void IntrusiveListPushBack(IntrusiveList* self, void* node);

void IntrusiveListPushFront(IntrusiveList* self, void* node);

void* IntrusiveListPopFront(IntrusiveList* self);

size_t IntrusiveListSize(IntrusiveList* self);

bool IntrusiveListIsEmpty(IntrusiveList* self);

/**
 * Other will be invalidated
 */
void IntrusiveListConcat(IntrusiveList* self, IntrusiveList* other);

/* Intrusive linked list */

IntrusiveLinkedList* IntrusiveLinkedListNew();

void IntrusiveLinkedListDelete(IntrusiveLinkedList* self);

void IntrusiveLinkedListPushBack(IntrusiveLinkedList* self, void* node);

void IntrusiveLinkedListPushFront(IntrusiveLinkedList* self, void* node);

void* IntrusiveLinkedListPopFront(IntrusiveLinkedList* self);

void* IntrusiveLinkedListPopBack(IntrusiveLinkedList* self);

size_t IntrusiveLinkedListSize(IntrusiveLinkedList* self);

bool IntrusiveLinkedListIsEmpty(IntrusiveLinkedList* self);


#endif //SCHED_LIST_H
