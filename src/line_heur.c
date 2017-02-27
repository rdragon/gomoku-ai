// heuristic evaluation function for alpha-beta

// this evaluation function returns a score based on the individual lines of the playing field (the `w` vertical lines, the `h` horizontal lines, and all of the diagonals). every line is evaluated seperately, for each of the two players. the score of the board is the weighted sum over all the line scores, with a weight of 1 for the active player and a weight of -1 for the opponent

// we assume n = 5 in the following explanation

//  black's score of a line (say) is based on how many possibilities black has of creating a five, and how many extra stones are needed for each possibility. more formal, we identify opportunities for black in the line. an opportunity for black is a segment of at least five adjacent squares containing at least one black stone, no white stones, and satisfying some more requirements explained below. some examples of opportunities for black are:
// (X = black stone, . = empty square)
// X...X  and  ...XX...  and  ....X
// in the first and third example only one five can be created, while in the second example four different fives can be created

// the requirements an opportunity for black needs to satisfy are:
//   1. the segment is at least 5 squares long, contains at least one black stone and contains no white stones
//   2. each black stone in the segment takes part in every five that can be constructed in the segment. for example, a segment like ....X.X.... or ...XX.X is not an opportunity for black (but these segments do contain opportunities)
//   3. each empty square in the segment takes part in at least one five that can be constructed in the segment. for example, the segment XXX.... is not an opportunity

// the score of an opportunity is based on its length and the number of stones contained in it. see the function `get_opportunity_score`
// black's score of a line is the sum of the scores of the opportunities for black contained in the line. when there are overlapping opportunities then we only count the one with the highest score

#include "shared.h"

#define east_id 0
#define south_id 1
#define se_id 2
#define ne_id 3

typedef struct Line
{
  int *mask;
  int value;
  int v;
  char length;
} Line;

int max_mask_length;
static int *three_power;
static Line *lines;
static int nlines;
static float *scores;
static int nscores;
static float multipliers[3];
int track_board_value;
float board_value;
float *board_values;

static void initialize_scores();

void file_line_heur_ini()
{
  int a, i;
  three_power = (int *)malloc_safe(sizeof(int) * (max_mask_length + 1));
  a = 1;
  for (i = 0; i <= max_mask_length; i++)
  {
    three_power[i] = a;
    a *= 3;
  }
  initialize_scores();
  nlines = size * 8;
  lines = (Line *)malloc_safe(nlines * sizeof(Line));
  board_values = (float *)malloc_safe(sizeof(float) * size);
}

void file_line_heur_cleanup()
{
  free(lines);
  free(scores);
  free(three_power);
  free(board_values);
}

static int get_line_index(int v, int id, int id_d)
{
  return v * 8 + (id - 1) * 4 + id_d;
}

static void free_masks()
{
  int i;
  for (i = 0; i < nlines; i++)
  {
    if (lines[i].value == 1)
    {
      free(lines[i].mask);
    }
  }
}

// returns the score of an opportunity of length `length` containing `stones` stones
static float get_opportunity_score(int stones, int length)
{
  switch (n)
  {
  case 3:
    switch (stones)
    {
    case 1:
      switch (length)
      {
      case 3: return 10;
      case 4: return 20;
      case 5: return 30;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    default:
      return 0;
    }
  case 4:
    switch (stones)
    {
    case 1:
      switch (length)
      {
      case 4: return 2;
      case 5: return 4;
      case 6: return 8;
      case 7: return 16;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    case 2:
      switch (length)
      {
      case 4: return 32;
      case 5: return 48;
      case 6: return 64;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    default:
      return 0;
    }
  case 5:
    switch (stones)
    {
    case 1:
      switch (length)
      {
      case 5: return 2;
      case 6: return 4;
      case 7: return 8;
      case 8: return 16;
      case 9: return 32;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    case 2:
      switch (length)
      {
      case 5: return 100;
      case 6: return 133;
      case 7: return 167;
      case 8: return 200;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    case 3:
      switch (length)
      {
      case 5: return 200;
      case 6: return 300;
      case 7: return 400;
      default: fail_("unexpected pattern of length %d with %d stones", length, stones); return 0;
      }
    default:
      return 0;
    }
  default:
    fail_("the lines heuristic does not support n = %d", n); return 0;
  }
}

// returns player `id`'s score of the line of length `length` starting with the indices `v`, `v + d`, `v + d + d`, ...
static float get_line_score(int v, int d, int length, int id)
{
  int k, l, m, x, y;
  int next_k;
  int next_min_start;
  float ans;
  float score;
  int start;  // index of the start square of the current opportunity
  int end;  // index of the last square of the current opportunity
  int last_stone;  // index of the last stone in the current opportunity
  int stones;  // number of stones in the current opportunity
  int last_end;  // index of the last square of the last opportunity
  int min_start;  // lower bound on start (for example because on position `min_start - 1` there lies a white stone)
  float last_score;  // score of the last opportunity. not yet added to `ans`
  ans = 0;
  last_end = -1;
  last_score = 0;
  min_start = 0;
  k = 0;
  while (k < length)
  {
    x = board[v + d * k];
    if (x == id)
    {
      stones = 1;
      next_k = -1;
      last_stone = k;
      m = min_(length, k + n);
      for (l = k + 1; l < m; l++)
      {
        y = board[v + d * l];
        if (y == id)
        {
          stones++;
          last_stone = l;
        }
        else if (!y)
        {
          // in the next opportunity we do not want any of the black stones in the first group (adjacent row of black stones) of this opportunity, since otherwise we could just as well add all the stones in this group to the next opportunity, and we would end up with again the same opportunity
          if (next_k == -1)
          {
            next_k = l + 1;
            next_min_start = l;
          }
        }
        else
        {
          break;
        }
      }
      // if `next_k == -1` then on position `l` there lies a white stone, or `l == length`
      if (next_k == -1)
      {
        next_k = l + 1;
        next_min_start = l + 1;
      }
      end = l - 1;
      start = max_(last_stone - n + 1, min_start);
      if (end - start + 1 >= n)
      {
        score = get_opportunity_score(stones, end - start + 1);
        if (score > 0)
        {
          if (last_score > 0 && start > last_end)
          {
            ans += last_score;
            last_score = 0;
          }
          if (score > last_score)
          {
            last_score = score;
            last_end = end;
          }
        }
      }
      k = next_k;
      min_start = next_min_start;
    }
    else
    {
      if (x)
      {
        min_start = k + 1;
      }
      k++;
    }
  }
  return ans + last_score;
}

// returns player `id`'s score of the board
static float get_board_score(int id)
{
  int x, y;
  float ans;
  ans = 0;
  for (y = 1; y < h - 1; y++)
  {
    ans += get_line_score(y * w + 1, 1, w - 2, id);
    if (y < h - n)
    {
      ans += get_line_score(y * w + 1, se, min_(w - 2, h - y - 1), id);
      ans += get_line_score(w - 2 + y * w, sw, min_(w - 2, h - y - 1), id);
    }
  }
  for (x = 1; x < w - 1; x++)
  {
    ans += get_line_score(x + w, w, h - 2, id);
    if (x > 1 && x < w - n)
    {
      ans += get_line_score(x + w, se, min_(h - 2, w - x - 1), id);
    }
    if (x < w - 2 && x > n - 1)
    {
      ans += get_line_score(x + w, sw, min_(h - 2, x), id);
    }
  }
  return ans;
}

// assumes `get_simple_move()` returns 0
float line_heur()
{
  float ans;
  if (track_board_value)
  {
    ans = board_value * (pid == active_id ? 1 : -1);
  }
  else
  {
    ans = (active_player->aggressiveness + 1) * get_board_score(pid) + (active_player->aggressiveness - 1) * get_board_score(3 - pid);
  }
  return ans + (float)rand() / RAND_MAX;
}

static void setup_line(int v, int id_d, int d, int line_length)
{
  int v_start, u, v_in, id, mask, len, i, j, *mask_pointer;
  Line *line;
  if (line_length < n)
  {
    return;
  }
  mask_pointer = 0;
  v_in = v;
  for (id = 1; id <= 2; id++)
  {
    mask = 0;
    len = 0;
    v = v_in;
    v_start = v;
    for (i = 0; i <= line_length; i++)
    {
      if (i == line_length || board[v] == 3 - id || board[v] == 3)
      {
        if (len >= n)
        {
          if (len <= max_mask_length)
          {
            mask_pointer = (int *)malloc_safe(sizeof(int));
            if (len < max_mask_length)
            {
              mask += three_power[len] * 2;
            }
            *mask_pointer = mask;
          }
          u = v_start;
          for (j = 0; j < len; j++)
          {
            line = &lines[get_line_index(u, id, id_d)];
            if (len <= max_mask_length)
            {
              line->mask = mask_pointer;
              line->value = three_power[j];
            }
            else
            {
              line->v = v_start;
              line->length = len;
            }
            u += d;
          }
        }
        mask = 0;
        len = 0;
        v_start = v + d;
      }
      else
      {
        if (board[v] == id)
        {
          mask += three_power[len];
        }
        len++;
      }
      v += d;
    }
  }
}

static void setup_lines()
{
  int x, y, i;
  for (i = 0; i < nlines; i++)
  {
    lines[i].value = 0;
    lines[i].length = 0;
  }
  for (y = 1; y < h - 1; y++)
  {
    setup_line(1 + w * y, east_id, 1, w - 2);
    setup_line(1 + w * y, se_id, se, min_(w - 2, h - y - 1));
    setup_line(1 + w * y, ne_id, ne, min_(w - 2, y));
  }
  for (x = 1; x < w - 1; x++)
  {
    setup_line(x + w, south_id, w, h - 2);
    if (x > 1)
    {
      setup_line(x + w, se_id, se, min_(w - x - 1, h - 2));
      setup_line(x + (h - 2) * w, ne_id, ne, min_(w - x - 1, h - 2));
    }
  }
}

// assumption: `!board[v]`
void update_board_value(int v, int stone)
{
  int id, id_d;
  int dirs[4] = { 1, w, 1 + w, 1 - w };
  Line *line;
  for (id = 1; id <= 2; id++)
  {
    for (id_d = 0; id_d < 4; id_d++)
    {
      line = &lines[get_line_index(v, id, id_d)];
      if (line->value)
      {
        board_value -= multipliers[id] * scores[*line->mask];
        *line->mask += line->value * (stone == id ? 1 : 2);
        board_value += multipliers[id] * scores[*line->mask];
      }
      else if (line->length)
      {
        board_value -= multipliers[id] * get_line_score(line->v, dirs[id_d], line->length, id);
        board[v] = stone;
        board_value += multipliers[id] * get_line_score(line->v, dirs[id_d], line->length, id);
        board[v] = 0;
      }
    }
  }
}

void revert_board_value(int v)
{
  int id, id_d;
  Line *line;
  for (id = 1; id <= 2; id++)
  {
    for (id_d = 0; id_d < 4; id_d++)
    {
      line = &lines[get_line_index(v, id, id_d)];
      if (line->value)
      {
        *line->mask -= line->value * (board[v] == id ? 1 : 2);
      }
    }
  }
}

static void initialize_scores()
{
  char *backup_board;
  int i, j, mask;
  nscores = three_power[max_mask_length];
  scores = (float *)malloc_safe(sizeof(float) * nscores);
  backup_board = board;
  board = (char *)malloc_safe(sizeof(char) * max_mask_length);
  for (i = 0; i < nscores; i++)
  {
    mask = i;
    for (j = 0; j < max_mask_length; j++)
    {
      board[j] = mask % 3;
      mask /= 3;
    }
    scores[i] = get_line_score(0, 1, max_mask_length, 1);
  }
  free(board);
  board = backup_board;
}

void start_tracking_board_value()
{
  multipliers[(int)active_id] = active_player->aggressiveness + 1;
  multipliers[3 - active_id] = active_player->aggressiveness - 1;
  setup_lines();
  board_value = multipliers[1] * get_board_score(1) + multipliers[2] * get_board_score(2);
  track_board_value = 1;
}

void stop_tracking_board_value()
{
  free_masks();
  track_board_value = 0;
}
