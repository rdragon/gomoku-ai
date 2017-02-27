// basic implementation of a growable array

#include "list.h"

#define max_(a, b) ((b) > (a) ? (b) : (a))
#define init_size 4

static void make_room_for_one_more(List *list)
{
  if (list->length == list->size)
  {
    list->size = max_(list->size * 2, init_size);
    list->values = (int *)realloc(list->values, list->size * sizeof(int));
    if (!list->values)
    {
      fprintf(stderr, "fatal error: realloc returned NULL (%s, line %d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
}

void list_ini(List *list, int size)
{
  list->length = 0;
  list->size = size;
  if (size)
  {
    list->values = (int *)malloc(size * sizeof(int));
    if (!list->values)
    {
      fprintf(stderr, "fatal error: malloc returned NULL (%s, line %d)\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    list->values = 0;
  }
}

void list_cleanup(List *list)
{
  free(list->values);
}

void list_clear(List *list)
{
  list->length = 0;
}

// adds `x` to the end of `list`
void list_add(List *list, int x)
{
  make_room_for_one_more(list);
  list->values[list->length++] = x;
}

// adds `x` to the end of `list` if `list` does not contain `x`
void list_include(List *list, int x)
{
  if (!list_contains(list, x))
  {
    list_add(list, x);
  }
}

// inserts `x` at position `i` in `list`
void list_insert(List *list, int x, int i)
{
  int j;
  if (i == list->length)
  {
    list_add(list, x);
  }
  else if (i >= 0 && i < list->length)
  {
    make_room_for_one_more(list);
    for (j = list->length - 1; j >= i; j--)
    {
      list->values[j + 1] = list->values[j];
    }
    list->values[i] = x;
    list->length++;
  }
  else
  {
    fprintf(stderr, "fatal error: invalid index `%d` as argument to `list_insert` (%s, line %d)\n", i, __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
}

// removes the last value from the list and returns this value
int list_pop(List *list)
{
  if (!list->length)
  {
    fprintf(stderr, "fatal error: `list_pop` on empty list (%s, line %d)\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  return list->values[--list->length];
}

// returns the last value of the list
int list_peak(List *list)
{
  if (!list->length)
  {
    fprintf(stderr, "fatal error: `list_peak` on empty list (%s, line %d)\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  return list->values[list->length - 1];
}

// returns the smallest index `i` such that `list->values[i] == x`. if no such index exists, then a value `-1` is returned
int list_find(List *list, int x)
{
  int i;
  for (i = 0; i < list->length; i++)
  {
    if (list->values[i] == x)
    {
      return i;
    }
  }
  return -1;
}

// returns whether `x` is present in `list`
int list_contains(List *list, int x)
{
  return list_find(list, x) != -1;
}

// removes the entry with index `i` from `list` (if `i` is an invalid index then nothing happens)
void list_remove_at(List *list, int i)
{
  int j;
  if (i < list->length && i >= 0)
  {
    for (j = i + 1; j < list->length; j++)
    {
      list->values[j - 1] = list->values[j];
    }
    list->length--;
  }
}

// removes the first occurrence of `x` from `list`
void list_remove(List *list, int x)
{
  list_remove_at(list, list_find(list, x));
}

// sorts the list in ascending order
void list_sort(List *list)
{
  int *values, k, n;
  if (!list->length)
  {
    return;
  }
  values = (int *)malloc(sizeof(int) * list->length);
  if (!values)
  {
    fprintf(stderr, "fatal error: malloc returned NULL (%s, line %d)\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  n = list->length;
  for (k = 0; k < n; k++)
  {
    values[k] = list->values[k];
  }
  list_clear(list);
  for (k = 0; k < n; k++)
  {
    list_ordered_add(list, values[k]);
  }
  free(values);
}

// sorts the list relative to `compare` in ascending order
void list_sort_custom(List *list, int(*compare)(int, int))
{
  int *values, k, n;
  if (!list->length)
  {
    return;
  }
  values = (int *)malloc(sizeof(int) * list->length);
  if (!values)
  {
    fprintf(stderr, "fatal error: malloc returned NULL (%s, line %d)\n", __FILE__, __LINE__);
    exit(EXIT_FAILURE);
  }
  n = list->length;
  for (k = 0; k < n; k++)
  {
    values[k] = list->values[k];
  }
  list_clear(list);
  for (k = 0; k < n; k++)
  {
    list_ordered_add_custom(list, values[k], compare);
  }
  free(values);
}

static int list_ordered_find_between(List *list, int x, int a, int b)
{
  int c;
  int y;
  if (a == b)
  {
    return a;
  }
  c = (b - a) / 2 + a;
  y = list->values[c];
  if (x == y)
  {
    return c;
  }
  else if (x > y)
  {
    return list_ordered_find_between(list, x, c + 1, b);
  }
  else
  {
    return list_ordered_find_between(list, x, a, c);
  }
}

static int list_ordered_find_between_custom(List *list, int i, int a, int b, int(*compare)(int, int))
{
  int c, p;
  if (a == b)
  {
    return a;
  }
  c = (b - a) / 2 + a;
  p = compare(i, list->values[c]);
  if (!p)
  {
    return c;
  }
  else if (p < 0)
  {
    return list_ordered_find_between_custom(list, i, a, c, compare);
  }
  else
  {
    return list_ordered_find_between_custom(list, i, c + 1, b, compare);
  }
}

// returns an index `i` such that `list->values[i] == x`. if no such index exists, then a value `i` with `0 <= i <= list->length` is returned such that `list->values[i - 1] <= x <= list->values[i]`
int list_ordered_find(List *list, int x)
{
  return list_ordered_find_between(list, x, 0, list->length);
}

// returns an index `i` such that `list->values[i] == x`. if no such index exists, then a value `i` with `0 <= i <= list->length` is returned such that `compare(list->values[i - 1], x)` and `compare(x, list->values[i])`
int list_ordered_find_custom(List *list, int i, int(*compare)(int, int))
{
  return list_ordered_find_between_custom(list, i, 0, list->length, compare);
}

// returns whether `x` is present in `list`
int list_ordered_contains(List *list, int x)
{
  int i;
  i = list_ordered_find(list, x);
  return i < list->length && list->values[i] == x;
}

// returns whether `i` is present in `list`
int list_ordered_contains_custom(List *list, int i, int(*compare)(int, int))
{
  int k;
  k = list_ordered_find_custom(list, i, compare);
  return k < list->length && !compare(i, list->values[k]);
}

// inserts `x` into `list`, while preserving the ascending order of the list (assuming the list is ordered by ascending values)
int list_ordered_add(List *list, int x)
{
  int i;
  i = list_ordered_find(list, x);
  list_insert(list, x, i);
  return i;
}

// inserts `i` into `list`, while preserving the ascending order of the list relative to `compare` (assuming the list has already an ascending order relative to `compare`)
int list_ordered_add_custom(List *list, int i, int(*compare)(int, int))
{
  int k;
  k = list_ordered_find_custom(list, i, compare);
  list_insert(list, i, k);
  return k;
}

// if `x` is not present in `list`, then it inserts `x` into `list`, while preserving the ascending order of the list (assuming the list is ordered by ascending values)
int list_ordered_include(List *list, int x)
{
  int i;
  i = list_ordered_find(list, x);
  if (i == list->length || list->values[i] != x)
  {
    list_insert(list, x, i);
  }
  return i;
}

// if `x` is not present in `list`, then it inserts `i` into `list`, while preserving the ascending order of the list relative to `compare` (assuming the list has already an ascending order relative to `compare`)
int list_ordered_include_custom(List *list, int i, int(*compare)(int, int))
{
  int k;
  k = list_ordered_find_custom(list, i, compare);
  if (k == list->length || compare(list->values[k], i))
  {
    list_insert(list, i, k);
  }
  return k;
}

// removes an occurrence of `x` from `list`
void list_ordered_remove(List *list, int x)
{
  list_remove_at(list, list_ordered_find(list, x));
}

// removes an occurrence of `i` from `list`
void list_ordered_remove_custom(List *list, int i, int(*compare)(int, int))
{
  list_remove_at(list, list_ordered_find_custom(list, i, compare));
}
