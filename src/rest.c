#include "shared.h"

int ai_random()
{
  if (verbose)
  {
    print_("playing a random move");
  }
  return random_empty_nearby_square();
}

// returns a move that either wins the game, counters a four-threat, creates two four-threats, or kicks-off the game. returns 0 if no such move exists
int ai_basic(int allow_counter)
{
  if (p->fives.length)
  {
    return p->fives.values[0];
  }
  else if (win_in_3(pid))
  {
    if (verbose)
    {
      print_("win in 3");
    }
    found_wts = 1;
    return q->fives.length ? q->fives.values[0] : p->double_fours.values[0];
  }
  else if (q->fives.length && allow_counter)
  {
    if (verbose && q->fives.length == 1)
    {
      print_("countering a four-threat of the opponent");
    }
    return q->fives.values[0];
  }
  else if (empty_squares == (w - 2) * (h - 2))
  {
    return w / 2 + w * (h / 2);
  }
  else
  {
    return 0;
  }
}

void fail()
{
  int backup;
  if (active_turn != turn)
  {
    fprintf(stderr, "temporary game at fail: ");
    backup = turn;
    turn = 0;
    print_game_state(stderr);
    turn = backup;
    undo_moves(turn - active_turn);
    fprintf(stderr, "active game at fail: ");
    print_game_state(stderr);
  }
  else
  {
    fprintf(stderr, "game at fail: ");
    print_game_state(stderr);
  }
  exit(EXIT_FAILURE);
}

void *calloc_safe(size_t num, size_t size)
{
  void *a = calloc(num, size);
  if (!a)
  {
    fail_("calloc_safe");
  }
  return a;
}

void *malloc_safe(size_t size)
{
  void *a = malloc(size);
  if (!a)
  {
    fail_("malloc_safe");
  }
  return a;
}

void *realloc_safe(void *ptr, size_t size)
{
  void *a = realloc(ptr, size);
  if (!a)
  {
    fail_("realloc_safe");
  }
  return a;
}

float get_time()
{
  return (float)clock() / CLOCKS_PER_SEC;
}

float time_left()
{
  return end_of_turn - get_time();
}

float get_elapsed_time()
{
  static float last_time = 0;
  float ans;
  ans = get_time() - last_time;
  last_time = get_time();
  return ans;
}

void set_time_limit(float duration)
{
  stop_time = get_time() + duration;
  out_of_time = 0;
}

int is_out_of_time()
{
  if (get_time() > stop_time)
  {
    out_of_time = 1;
  }
  return out_of_time;
}

void on_wts()
{
  if (halt_on_wts && !manual_steps && no_human_player())
  {
    manual_steps = 1;
    if (!show_board)
    {
      show_board = 1;
      if (json_client)
      {
        send_board();
      }
    }
    if (verbose)
    {
      printf("switched to manual steps (turn off with \"--no-halt-on-wts\")\n");
      if (json_client)
      {
        printf("press enter to start the next turn\n");
      }
    }
  }
}

float truncate(float x, float a, float b)
{
  if (x < a)
  {
    return a;
  }
  else if (x > b)
  {
    return b;
  }
  else
  {
    return x;
  }
}

int get_d(int v, int u)
{
  int x, y;
  if (v == u)
  {
    return 0;
  }
  x = u % w - v % w;
  y = u / w - v / w;
  if (abs(x) >= n || abs(y) >= n)
  {
    return 0;
  }
  if (x && y && x - y && x + y)
  {
    return 0;
  }
  if (x)
  {
    return x / abs(x) + w * (y / abs(x));
  }
  else
  {
    return w * (y / abs(y));
  }
}

void print_time(float duration, const char *name)
{
  if (duration > min_(p->time_limit * 0.1, 0.1) && verbose)
  {
    print_("%s took %.2f sec", name, duration)
  }
}
