#pragma once

#define DECLARE_LINKED_LIST_TYPE(element_type, container_type)                                                    \
  struct container_type                                                                                           \
  {                                                                                                               \
    element_type* head;                                                                                           \
    element_type* tail;                                                                                           \
  }

#define DECLATE_INTRUSIVE_LL_MEMBERS(element_type)                                                                \
  element_type* next;                                                                                             \
  element_type* prev;

#define LINKED_LIST_APPEND(container, value)                                                                      \
  do {                                                                                                            \
    assert(!(value)->next && !(value)->prev);                                                                     \
    (value)->next = NULL; (value)->prev = (container)->tail;                                                      \
    if ((container)->tail) (container)->tail->next = (value);                                                     \
    if (!(container)->head) (container)->head = (value);                                                          \
    (container)->tail = (value);                                                                                  \
  } while (false)

#define LINKED_LIST_REMOVE(container, value)                                                                      \
  do {                                                                                                            \
    if ((container)->head == (value)) (container)->head = (value)->next;                                          \
    if ((container)->tail == (value)) (container)->tail = (value)->prev;                                          \
    if ((value)->prev) (value)->prev->next = (value)->next;                                                       \
    if ((value)->next) (value)->next->prev = (value)->prev;                                                       \
    (value)->next = (value)->prev = NULL;                                                                         \
  } while (false)

#define LINKED_LIST_GET_HEAD(container) (container)->head
#define LINKED_LIST_GET_TAIL(container) (container)->tail
#define LINKED_LIST_GET_NEXT(value) (value)->next
#define LINKED_LIST_GET_PREV(value) (value)->prev
