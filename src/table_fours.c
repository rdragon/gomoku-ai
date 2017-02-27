#include "shared.h"

#define places 40 // sizeof(ullong) * 8 * log(2) / log(3)
#define init_size 32
#define maxlen 8
#define mod (1 << 24)
#define masklen (index + 1)

static int *table;
static ullong **masks;
static int *sizes;
static int *lengths;
static ullong *mask;
static int index;
static ullong *pow3;
static int counter;
static int *nums;
static uint sum;
static uint *randoms;
static int stats_maxchain;
static int done;

static void get_win(int);

static void copy_mask(ullong *dest, ullong *source)
{
  int i;
  for (i = 0; i < masklen; i++)
  {
    dest[i] = source[i];
  }
}

static int compare_masks(ullong *a, ullong *b)
{
  int i;
  for (i = 0; i < masklen; i++)
  {
    if (a[i] != b[i])
    {
      return 1;
    }
  }
  return 0;
}

static void add_v(int v, int id)
{
  int k;
  if (nums[v] == -1)
  {
    nums[v] = counter++;
  }
  k = nums[v] / places;
  if (k == maxlen)
  {
    fail_("overflow");
  }
  index = max_(index, k);
  mask[k] += pow3[nums[v] % places] * id;
  sum += (3 - 2 * id) * randoms[v];
}

static void remove_v(int v, int id)
{
  int k;
  k = nums[v] / places;
  mask[k] -= pow3[nums[v] % places] * id;
  sum -= (3 - 2 * id) * randoms[v];
  if (k && k == index && !mask[k])
  {
    index--;
    while (index && !mask[index])
    {
      index--;
    }
  }
}

static void make_room()
{
  if (lengths[index] == sizes[index])
  {
    sizes[index] = max_(init_size, sizes[index] * 2);
    masks[index] = (ullong *)realloc_safe(masks[index], sizes[index] * masklen * sizeof(ullong));
  }
}

static int add_mask()
{
  uint x, s;
  int i, j, chain;
  s = sum & (mod - 1);
  x = table[s];
  chain = 1;
  while (x)
  {
    i = x & (maxlen - 1);
    j = x / maxlen;
    if (i == index && !compare_masks(mask, &masks[index][masklen * j]))
    {
      return 0;
    }
    s = (s + 1) & (mod - 1);
    x = table[s];
    chain++;
    if (chain == mod)
    {
      fail_("hash table overflow");
    }
    stats_maxchain = max_(stats_maxchain, chain);
  }
  make_room();
  j = lengths[index]++;
  table[s] = index | j * maxlen;
  copy_mask(&masks[index][masklen * j], mask);
  return 1;
}

static void handle_four(int v, int u, int depth)
{
  if (done)
  {
    return;
  }
  add_v(v, 1);
  add_v(u, 2);
  if (add_mask())
  {
    submit_move(v);
    if (!win_in_3(pid))
    {
      submit_move(u);
      get_win(depth + 1);
      if (result.success)
      {
        result.threats[depth].gv = v;
        result.threats[depth].cvs[0] = u;
        result.threats[depth].cv_count = 1;
      }
      undo_move();
    }
    undo_move();
  }
  remove_v(u, 2);
  remove_v(v, 1);
}

static void get_win(int depth)
{
  int v, u, i;
  if (is_out_of_time())
  {
    done = 1;
  }
  else if (win_in_3(pid))
  {
    v = q->fives.length ? q->fives.values[0] : p->double_fours.values[0];
    result.success = 1;
    result.threat_count = depth + 1;
    result.threats = (ResultThreat *)malloc_safe(result.threat_count * sizeof(ResultThreat));
    result.threats[depth].gv = v;
    result.threats[depth].cvs[0] = p->fours[v].values[0];
    result.threats[depth].cv_count = 1;
    result.win_vec = p->fours[v].values[1];
    result.only_fours = 1;
    result.table = 1;
    done = 1;
  }
  else if (q->fives.length)
  {
    v = q->fives.values[0];
    if (p->fours[v].length)
    {
      handle_four(v, p->fours[v].values[0], depth);
    }
  }
  else
  {
    for (i = 0; i < p->fl.length; i++)
    {
      v = p->fl.values[i];
      u = p->fours[v].values[0];
      handle_four(v, u, depth);
      handle_four(u, v, depth);
    }
  }
}

static uint get_rand()
{
  uint x, y;
  int i;
  y = mod;
  x = 0;
  i = 0;
  while (y > 1)
  {
    x |= ((uint)rand() & 32767) << i;
    i += 15;
    y >>= 15;
  }
  return x;
}

int table_fours(char id)
{
  int i, v, m;
  char pid_before;
  ullong x;
  pid_before = set_p(id);
  result.success = 0;
  stats_maxchain = 1;
  sum = 0;
  counter = 0;
  nums = (int *)malloc_safe(sizeof(int) * size);
  randoms = (uint *)malloc_safe(sizeof(uint) * size);
  for (v = v0; v < v1; v++)
  {
    nums[v] = -1;
    randoms[v] = get_rand();
  }
  pow3 = (ullong *)malloc_safe(places * sizeof(ullong));
  x = 1;
  for (i = 0; i < places; i++)
  {
    pow3[i] = x;
    x *= 3;
  }
  masks = (ullong **)calloc_safe(maxlen, sizeof(ullong *));
  sizes = (int *)calloc_safe(maxlen, sizeof(int));
  lengths = (int *)calloc_safe(maxlen, sizeof(int));
  mask = (ullong *)calloc_safe(maxlen, sizeof(ullong));
  table = (int *)calloc_safe(mod, sizeof(int));
  index = 0;
  make_room();
  lengths[0]++;
  done = 0;
  get_win(0);
  if (1)
  {
    m = 0;
    for (i = 0; i < maxlen; i++)
    {
      m += lengths[i];
    }
    print_("table_fours stats: maxchain = %d, count = %d, masks = %d", stats_maxchain, counter, m);
  }
  free(nums);
  free(randoms);
  free(pow3);
  for (i = 0; i < maxlen; i++)
  {
    free(masks[i]);
  }
  free(masks);
  free(sizes);
  free(lengths);
  free(mask);
  free(table);
  set_p(pid_before);
  return result.success;
}
