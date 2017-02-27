#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdio.h>

typedef struct List
{
  int *values;
  int length;
  int size;
} List;

void list_ini(List *, int);
void list_cleanup(List *);
void list_clear(List *);
void list_add(List *, int);
void list_include(List *, int);
void list_insert(List *, int, int);
int list_pop(List *);
int list_peak(List *);
int list_find(List *, int);
int list_contains(List *, int);
void list_remove_at(List *, int);
void list_remove(List *, int);
void list_sort(List *);
void list_sort_custom(List *, int(*)(int, int));
int list_ordered_find(List *, int);
int list_ordered_find_custom(List *, int, int(*)(int, int));
int list_ordered_contains(List *, int);
int list_ordered_contains_custom(List *, int, int(*)(int, int));
int list_ordered_add(List *, int);
int list_ordered_add_custom(List *, int, int(*)(int, int));
int list_ordered_include(List *, int);
int list_ordered_include_custom(List *, int, int(*)(int, int));
void list_ordered_remove(List *, int);
void list_ordered_remove_custom(List *, int, int(*)(int, int));

#endif
