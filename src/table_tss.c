#include "shared.h"

#define bits 64 // number of bits in a long long
#define init_size 32
#define ncompares 2

typedef struct Map
{
  long long *masks;
  List *sumlists;
  int size;
  int length;
} Map;

static Map *maps;
static Map *map;
static long long *mask;
static List *lists;
static List *list;
static int gv_counter;
static int *nums;
static unsigned int sum;
static unsigned int *randoms;
static int masklen;
static int stats_maxsumlist;
static int done;
static int success;

static void get_win();

static void make_room()
{
  if (map->length == map->size)
  {
    map->size = max_(map->size * 2, init_size);
    map->masks = (long long *)realloc_safe(map->masks, map->size * masklen * sizeof(long long));
    map->sumlists = (List *)realloc_safe(map->sumlists, map->size * sizeof(List));
    mask = map->masks;
  }
}

static void set_mask(long long *dest, long long *source, int source_len)
{
  int i;
  for (i = 0; i < source_len; i++)
  {
    dest[i] = source[i];
  }
  for (; i < masklen; i++)
  {
    dest[i] = 0;
  }
}

static void set_masklen(int len)
{
  masklen = len;
  map = &maps[masklen - 1];
  list = &lists[masklen - 1];
  mask = map->masks;
}

static void add_gv(int v)
{
  int num, k, oldlen;
  long long *oldmask;
  if (nums[v] == -1)
  {
    nums[v] = gv_counter++;
  }
  num = nums[v];
  k = num / bits;
  if (k >= masklen)
  {
    oldlen = masklen;
    oldmask = mask;
    set_masklen(k + 1);
    if (!map->length)
    {
      make_room();
      map->length++;
    }
    set_mask(mask, oldmask, oldlen);
  }
  mask[k] |= 1LL << (num % bits);
}

static void remove_gv(int v)
{
  int num, k;
  num = nums[v];
  k = num / bits;
  mask[k] &= ~(1LL << (num % bits));
  if (!mask[k] && k + 1 == masklen && k)
  {
    masklen = k;
    while (masklen > 1 && !mask[masklen - 1])
    {
      masklen--;
    }
    set_masklen(masklen);
  }
}

static int compare1(int i, int j)
{
  if (mask[i] == mask[j])
  {
    return 0;
  }
  else if (mask[i] < mask[j])
  {
    return -1;
  }
  else
  {
    return 1;
  }
}

static int compare2(int i, int j)
{
  i *= 2;
  j *= 2;
  if (mask[i] == mask[j])
  {
    if (mask[i + 1] == mask[j + 1])
    {
      return 0;
    }
    else if (mask[i + 1] < mask[j + 1])
    {
      return -1;
    }
    else
    {
      return 1;
    }
  }
  else if (mask[i] < mask[j])
  {
    return -1;
  }
  else
  {
    return 1;
  }
}

static int compare(int i, int j)
{
  int k;
  for (k = 0; k < masklen; k++)
  {
    if (mask[i * masklen + k] < mask[j * masklen + k])
    {
      return -1;
    }
    else if (mask[i * masklen + k] > mask[j * masklen + k])
    {
      return 1;
    }
  }
  return 0;
}

static int add_mask()
{
  int p, i;
  List *sumlist;
  if (masklen == 1)
  {
    p = list_ordered_find_custom(list, 0, compare1);
  }
  else if (masklen == 2)
  {
    p = list_ordered_find_custom(list, 0, compare2);
  }
  else
  {
    p = list_ordered_find_custom(list, 0, compare);
  }
  if (p < list->length && !compare(list->values[p], 0))
  {
    i = list->values[p];
    sumlist = &map->sumlists[i];
    if (list_contains(sumlist, sum))
    {
      return 0;
    }
    else
    {
      list_add(sumlist, sum);
      stats_maxsumlist = max_(sumlist->length, stats_maxsumlist);
    }
  }
  else
  {
    i = map->length;
    list_insert(list, i, p);
    make_room();
    set_mask(&mask[i * masklen], mask, masklen);
    list_ini(&map->sumlists[i], 1);
    list_add(&map->sumlists[i], sum);
    map->length++;
  }
  return 1;
}

static void handle_three(int v, int count, int *cvs)
{
  int oldsum;
  add_gv(v);
  oldsum = sum;
  sum += randoms[cvs[0]] + randoms[cvs[1]] + randoms[cvs[2]];
  if (add_mask())
  {
    submit_move(v);
    if (!win_in_3(pid))
    {
      submit_moves(count, cvs);
      get_win();
      undo_moves(count);
    }
    undo_move();
  }
  sum = oldsum;
  remove_gv(v);
}

static void loop_threes(int v)
{
  int a, b, x, y, z, d, left, right, ll, rr, dir;
  int dirs[] = { 1, w, 1 + w, 1 - w };
  int cvs[3];
  for (dir = 0; dir < 4; dir++)
  {
    d = dirs[dir];
    a = tail_length(v, d, pid);
    b = tail_length(v, -d, pid);
    x = 1 + a + b;
    right = v + (a + 1) * d;
    left = v - (b + 1) * d;
    if (!board[right])
    {
      y = min_(1 + tail_length(right, d, pid), n - x);
    }
    else
    {
      y = 0;
    }
    if (!board[left])
    {
      z = min_(1 + tail_length(left, -d, pid), n - x);
    }
    else
    {
      z = 0;
    }
    if (!board[right] && !board[left] && x + y < n && x + z < n
      && (x + y == n - 1 || x + z == n - 1))
    {
      rr = v + (a + y + 1) * d;
      ll = v - (b + z + 1) * d;
      if (x + y == n - 1 && !board[rr])
      {
        y++;
      }
      else
      {
        y = 1;
      }
      if (x + z == n - 1 && !board[ll])
      {
        z++;
      }
      else
      {
        z = 1;
      }
      if (x + y == n && x + z == n)
      {
        cvs[0] = left;
        cvs[1] = right;
        cvs[2] = 0;
        handle_three(v, 2, cvs);
      }
      else if (x + y == n)
      {
        cvs[0] = right;
        cvs[1] = left;
        cvs[2] = rr;
        handle_three(v, 3, cvs);
      }
      else if (x + z == n)
      {
        cvs[0] = left;
        cvs[1] = right;
        cvs[2] = ll;
        handle_three(v, 3, cvs);
      }
    }
    if (done)
    {
      break;
    }
  }
}

static void handle_four(int v, int u)
{
  add_gv(v);
  sum += randoms[u];
  if (add_mask())
  {
    submit_move(v);
    if (!win_in_3(pid))
    {
      submit_move(u);
      get_win();
      undo_move();
    }
    undo_move();
  }
  sum -= randoms[u];
  remove_gv(v);
}

static void get_win()
{
  int u, v;
  if (is_out_of_time())
  {
    done = 1;
    return;
  }
  if (win_in_3(pid))
  {
    success++;//temp
    done = 1;
    return;
  }
  v = q->fives.length ? q->fives.values[0] : v0;
  for (; v < v1; v++)
  {
    if (board[v])
    {
      continue;
    }
    if (p->fours[v].length)
    {
      u = p->fours[v].values[0];
      handle_four(v, u);
    }
    if (!done)
    {
      loop_threes(v);
    }
    if (q->fives.length || done)
    {
      break;
    }
  }
}

int table_tss()
{
  int i, j, v, maxlen, masks;
  result.success = 0;
  stats_maxsumlist = 1;
  sum = 0;
  gv_counter = 0;
  maxlen = max_(1, ((empty_squares + 1) / 2 + bits - 1) / bits);
  nums = (int *)malloc_safe(sizeof(int) * size);
  randoms = (unsigned int *)malloc_safe(sizeof(unsigned int) * size);
  randoms[0] = 0;
  for (v = v0; v < v1; v++)
  {
    nums[v] = -1;
    randoms[v] = (unsigned int)rand();
  }
  lists = (List *)calloc_safe(maxlen, sizeof(List));
  maps = (Map *)calloc_safe(maxlen, sizeof(Map));
  set_masklen(1);
  make_room();
  map->length++;
  *mask = 0;
  done = 0;
  success = 0;
  get_win();
  if (1)
  {
    masks = 0;
    for (i = 0; i < maxlen; i++)
    {
      masks += lists[i].length;
    }
    print_("table_fours stats: maxsumlist = %d, count = %d, masks = %d, success = %d", stats_maxsumlist, gv_counter, masks, success);
  }
  free(nums);
  free(randoms);
  for (i = 0; i < maxlen; i++)
  {
    list_cleanup(&lists[i]);
    free(maps[i].masks);
    for (j = 1; j < maps[i].length; j++)
    {
      list_cleanup(&maps[i].sumlists[j]);
    }
    free(maps[i].sumlists);
  }
  free(lists);
  free(maps);
  return success;
}
