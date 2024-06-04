#include <stdio.h>
#include "list.h"

/* Intrusive node */

bool IntrusiveNodeIsLinked(IntrusiveNode* node) {
    return node->next != NULL;
}

void IntrusiveNodeUnlink(IntrusiveNode* node) {
    if (node->next) {
        node->next->prev = node->prev;
    }

    if (node->prev) {
        node->prev->next = node->next;
    }

    node->next = NULL;
    node->prev = NULL;
}

/* Intrusive list */

IntrusiveList* IntrusiveListNew() {
    IntrusiveList* list = (IntrusiveList*)calloc(1, sizeof(IntrusiveList));
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void IntrusiveListDelete(IntrusiveList* self) {
    IntrusiveNode* it = self->head;
    while (it != NULL) {
        IntrusiveNode* next = it->next;
        free(it);
        it = next;
    }
    self->head = NULL;
    self->tail = NULL;
    self->size = 0;
    free(self);
    self = NULL;
}

void IntrusiveListPushBack(IntrusiveList* self, void* node) {
    assert(self);

    IntrusiveNode* tag = (IntrusiveNode*)node;

    self->size += 1;

    tag->next = NULL;

    if (IntrusiveListIsEmpty(self)) {
        self->head = self->tail = tag;
    } else {
        self->tail->next = tag;
        self->tail = tag;
    }

//    printf("pushed to global\n");
//    fflush(stdout);
}

void IntrusiveListPushFront(IntrusiveList* self, void* node) {
    assert(self);

    IntrusiveNode* tag = (IntrusiveNode*)node;

    self->size += 1;

    if (IntrusiveListIsEmpty(self)) {
        self->head = self->tail = tag;
    } else {
        tag->next = self->head;
        self->head = tag;
    }
}

void* IntrusiveListPopFront(IntrusiveList* self) {
    assert(self);

    if (IntrusiveListIsEmpty(self)) {
        return NULL;
    }
    self->size -= 1;

    IntrusiveNode* front = self->head;

    if (self->head == self->tail) {
        self->head = self->tail = NULL;
    } else {
        self->head = self->head->next;
    }

    front->next = NULL;

    return front;
}

size_t IntrusiveListSize(IntrusiveList* self) {
    assert(self);
    return self->size;
}

bool IntrusiveListIsEmpty(IntrusiveList* self) {
    assert(self);
    return self->head == NULL;
}

/**
 * Other will be invalidated
 */
void IntrusiveListConcat(IntrusiveList* self, IntrusiveList* other) {
    if (IntrusiveListIsEmpty(other)) {
        return;
    }

    if (IntrusiveListIsEmpty(self)) {
        self->head = other->head;
        self->tail = other->tail;
        self->size = other->size;
    } else {
        self->tail->next = other->head;
        self->tail = other->tail;
        self->size += other->size;
    }

    other->head = NULL;
    other->tail = NULL;
    other->size = 0;
}

/* Intrusive linked list */
IntrusiveLinkedList* IntrusiveLinkedListNew();

void IntrusiveLinkedListDelete(IntrusiveLinkedList* self);

void IntrusiveLinkedListPushBack(IntrusiveLinkedList* self, void* node);

void IntrusiveLinkedListPushFront(IntrusiveLinkedList* self, void* node);

void* IntrusiveLinkedListPopFront(IntrusiveLinkedList* self);

void* IntrusiveLinkedListPopBack(IntrusiveLinkedList* self);

size_t IntrusiveLinkedListSize(IntrusiveLinkedList* self);

bool IntrusiveLinkedListIsEmpty(IntrusiveLinkedList* self);


// -----------------------------------

IntrusiveLinkedList* IntrusiveLinkedListNew() {
    return IntrusiveListNew();
}

void IntrusiveLinkedListDelete(IntrusiveLinkedList* self) {
    IntrusiveListDelete(self);
}

void IntrusiveLinkedListPushBack(IntrusiveLinkedList* self, void* node) {
    IntrusiveNode* tag = (IntrusiveNode*) node;
    tag->prev = self->tail;

    IntrusiveListPushBack(self, node);
}

void IntrusiveLinkedListPushFront(IntrusiveLinkedList* self, void* node) {
    IntrusiveNode* tag = (IntrusiveNode*) node;

    self->head->prev = tag;

    IntrusiveListPushFront(self, node);
}

void* IntrusiveLinkedListPopFront(IntrusiveLinkedList* self) {
    self->head->prev = NULL;

    return IntrusiveListPopFront(self);
}

void* IntrusiveLinkedListPopBack(IntrusiveLinkedList* self) {
    assert(self);

    if (IntrusiveListIsEmpty(self)) {
        return NULL;
    }
    self->size -= 1;

    IntrusiveNode* back = self->tail;

    if (self->head == self->tail) {
        self->head = self->tail = NULL;
    } else {
        self->tail = self->tail->prev;
    }

    back->prev = NULL;

    return back;
}

size_t IntrusiveLinkedListSize(IntrusiveLinkedList* self) {
    return IntrusiveListSize(self);
}

bool IntrusiveLinkedListIsEmpty(IntrusiveLinkedList* self) {
    return IntrusiveListIsEmpty(self);
}