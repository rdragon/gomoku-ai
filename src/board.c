#include "shared.h"

#define action_create_four 0
#define action_set_winner 1

void board_clear(char *board)
{
  int v, i, j;
  v = v0;
  for (i = 2; i < h; i++)
  {
    for (j = 2; j < w; j++)
    {
      board[v++] = 0;
    }
    v += 2;
  }
}

// returns how many squares with value `id` exist before a square with some other value occurs in the sequence v+d, v+d+d, v+d+d+d, ...
int tail_length(int v, int d, char id)
{
  int len;
  len = 0;
  v += d;
  while (v >= 0 && v < size && board[v] == id)
  {
    len++;
    v += d;
  }
  return len;
}

static void update_nearby_full(int v, int d)
{
  int u, u0, i;
  u0 = v + 2 * nw;
  for (i = 0; i < 5; i++)
  {
    for (u = u0; u < u0 + 5; u++)
    {
      nearby[u] += d;
    }
    u0 += w;
  }
}

static void update_nearby_partial(int v_x, int v_y, int d)
{
  int x0, x1, y0, y1, x, y;
  x0 = max_(1, v_x - 2);
  x1 = min_(w - 1, v_x + 3);
  y0 = max_(1, v_y - 2);
  y1 = min_(h - 1, v_y + 3);
  for (y = y0; y < y1; y++)
  {
    for (x = x0; x < x1; x++)
    {
      nearby[x + w * y] += d;
    }
  }
}

// executes `nearby[s] += d` for each square `s` in the 5x5 subfield with center square `v`
void update_nearby(int v, int d)
{
  int x, y;
  x = v % w;
  y = v / w;
  if (x > 1 && x < w - 2 && y > 1 && y < h - 2)
  {
    update_nearby_full(v, d);
  }
  else
  {
    update_nearby_partial(x, y, d);
  }
}

static void add_four(Player *r, int v, int u)
{
  list_add(&r->fours[v], u);
  if (r->fours[v].length == 2)
  {
    list_add(&r->double_fours, v);
  }
}

static void remove_four(Player *r, int v, int u)
{
  list_remove(&r->fours[v], u);
  if (r->fours[v].length == 1)
  {
    list_remove(&r->double_fours, v);
  }
}

static void add_five(Player *r, int v)
{
  if (!r->five_count[v])
  {
    list_add(&r->fives, v);
  }
  r->five_count[v]++;
}

static void remove_five(Player *r, int v)
{
  r->five_count[v]--;
  if (!r->five_count[v])
  {
    list_remove(&r->fives, v);
  }
}

static void fl_add(Player *r, int v)
{
  if (!r->fl_count[v]++)
  {
    list_ordered_add(&r->fl, v);
  }
}

static void fl_remove(Player *r, int v)
{
  if (!--r->fl_count[v])
  {
    list_ordered_remove(&r->fl, v);
  }
}

static void create_four(int v, int u)
{
  if (!list_contains(&p->fours[v], u))
  {
    add_four(p, v, u);
    add_four(p, u, v);
    list_add(&actions, u);
    list_add(&actions, v);
    list_add(&actions, action_create_four);
    list_add(&actions, turn);
    fl_add(p, min_(u, v));
  }
}

static void create_fours(int v)
{
  int dir, j, a, b, x, y, left, right, left2, right2, u, d;
  int dirs[4] = { 1, w, 1 + w, 1 - w };
  for (dir = 0; dir < 4; dir++)
  {
    d = dirs[dir];
    a = tail_length(v, d, pid);
    b = tail_length(v, -d, pid);
    x = 1 + a + b;
    if (x > n - 2)
    {
      continue;
    }
    right = v + (a + 1) * d;
    left = v - (b + 1) * d;
    y = 0; // will eventually contain the number of `pid` stones directly to the right of `right` and to the left of `left`, when `left` and `right` are empty
    if (!board[right])
    {
      right2 = 0; // when non-zero then `right2` is empty and only `pid` stones lie between `right` and `right2`
      u = right + d;
      for (j = x + 1; j < n; j++)
      {
        if (!board[u])
        {
          if (right2)
          {
            break;
          }
          else
          {
            right2 = u;
          }
        }
        else if (board[u] == pid)
        {
          if (!right2)
          {
            y++;
          }
        }
        else
        {
          break;
        }
        u += d;
      }
      if (j == n && right2)
      {
        create_four(right, right2);
      }
    }
    if (!board[left])
    {
      left2 = 0;
      u = left - d;
      for (j = x + 1; j < n; j++)
      {
        if (!board[u])
        {
          if (left2)
          {
            break;
          }
          else
          {
            left2 = u;
          }
        }
        else if (board[u] == pid)
        {
          if (!left2)
          {
            y++;
          }
        }
        else
        {
          break;
        }
        u -= d;
      }
      if (j == n && left2)
      {
        create_four(left, left2);
      }
      if (x + y >= n - 2 && !board[right])
      {
        create_four(left, right);
      }
    }
  }
}

static void upgrade_fours(int v)
{
  int i, u;
  for (i = 0; i < p->fours[v].length; i++)
  {
    u = p->fours[v].values[i];
    remove_four(p, u, v);
    add_five(p, u);
    fl_remove(p, min_(u, v));
  }
}

static void downgrade_fives(int v)
{
  int i, u;
  for (i = 0; i < p->fours[v].length; i++)
  {
    u = p->fours[v].values[i];
    remove_five(p, u);
    add_four(p, u, v);
    fl_add(p, min_(u, v));
  }
}

static void remove_fours_q(int v)
{
  int i, u;
  for (i = 0; i < q->fours[v].length; i++)
  {
    u = q->fours[v].values[i];
    remove_four(q, u, v);
    fl_remove(q, min_(u, v));
  }
}

static void add_fours_q(int v)
{
  int i, u;
  for (i = 0; i < q->fours[v].length; i++)
  {
    u = q->fours[v].values[i];
    add_four(q, u, v);
    fl_add(q, min_(u, v));
  }
}

static void update_threats(int v)
{
  create_fours(v);
  upgrade_fours(v);
  remove_fours_q(v);
  if (p->fours[v].length >= 2)
  {
    list_remove(&p->double_fours, v);
  }
  if (q->fours[v].length >= 2)
  {
    list_remove(&q->double_fours, v);
  }
  if (q->five_count[v])
  {
    list_remove(&q->fives, v);
  }
}

static void revert_threats(int v)
{
  downgrade_fives(v);
  add_fours_q(v);
  if (p->fours[v].length >= 2)
  {
    list_add(&p->double_fours, v);
  }
  if (q->fours[v].length >= 2)
  {
    list_add(&q->double_fours, v);
  }
  if (q->five_count[v])
  {
    list_add(&q->fives, v);
  }
}

static void set_winner(char id)
{
  winner = id;
  list_add(&actions, action_set_winner);
  list_add(&actions, turn);
}

// places a stone on the board for the current player and gives the turn to the other player
void submit_move(int v)
{
  if (sanity_checks)
  {
    if (v < 0 || v >= size)
    {
      fail_("submit_move: invalid position (v = %d)", v);
    }
    if (board[v])
    {
      fail_("submit_move: (%d, %d) is a non-empty square (stone = %d)", v % w - 1, v / w - 1, board[v]);
    }
  }
  if (track_board_value)
  {
    board_values[turn] = board_value;
    update_board_value(v, pid);
  }
  board[v] = pid;
  empty_squares--;
  if (!winner)
  {
    if (list_contains(&p->fives, v))
    {
      set_winner(pid);
    }
    else if (!empty_squares)
    {
      set_winner(draw);
    }
  }
  update_nearby(v, 1);
  update_threats(v);
  moves[turn] = pid == 1 ? v : -v;
  turn++;
  set_p(3 - pid);
}

// places `k` stones on the board for the current player and gives the turn to the other player
void submit_moves(int k, int *ar)
{
  int i;
  for (i = 0; i < k; i++)
  {
    submit_move(ar[i]);
    if (i + 1 < k)
    {
      set_p(3 - pid);
    }
  }
}

// reverts all actions done in the current turn
static void revert_actions()
{
  int u, v, action;
  while (actions.length && list_peak(&actions) == turn)
  {
    actions.length--;
    action = list_pop(&actions);
    if (action == action_create_four)
    {
      v = list_pop(&actions);
      u = list_pop(&actions);
      remove_four(p, v, u);
      remove_four(p, u, v);
      fl_remove(p, min_(u, v));
    }
    else if (action == action_set_winner)
    {
      winner = 0;
    }
  }
}

// undoes the last move
void undo_move()
{
  int v;
  if (!turn)
  {
    fail_("undo_move: no moves to undo");
  }
  turn--;
  empty_squares++;
  v = abs(moves[turn]);
  set_p(moves[turn] > 0 ? 1 : 2);
  if (track_board_value)
  {
    revert_board_value(v);
    board_value = board_values[turn];
  }
  board[v] = 0;
  update_nearby(v, -1);
  revert_threats(v);
  revert_actions();
}

// undoes a number of moves
void undo_moves(int k)
{
  int i;
  for (i = 0; i < k; i++)
  {
    undo_move();
  }
}

// returns a random empty square, or 0 if no such square exists
int random_empty_square()
{
  int v;
  List list;
  list_ini(&list, empty_squares);
  for (v = v0; v < v1; v++)
  {
    if (!board[v])
    {
      list_add(&list, v);
    }
  }
  if (list.length)
  {
    v = list.values[rand() % list.length];
  }
  else
  {
    v = 0;
  }
  list_cleanup(&list);
  return v;
}

// returns a random empty square at most 2 squares away from a player stone on the board, or `random_empty_square()` if no such square exists
int random_empty_nearby_square()
{
  int v;
  List list;
  list_ini(&list, empty_squares);
  for (v = v0; v < v1; v++)
  {
    if (!board[v] && nearby[v])
    {
      list_add(&list, v);
    }
  }
  if (list.length)
  {
    v = list.values[rand() % list.length];
  }
  else
  {
    v = random_empty_square();
  }
  list_cleanup(&list);
  return v;
}

// returns a move that either wins the game, counters a four-threat, or creates two four-threats. returns 0 if no such move exists
int get_simple_move()
{
  if (p->fives.length)
  {
    return p->fives.values[0];
  }
  else if (q->fives.length)
  {
    return q->fives.values[0];
  }
  else if (p->double_fours.length)
  {
    return p->double_fours.values[0];
  }
  else
  {
    return 0;
  }
}

char set_p(char id)
{
  char pid_before;
  pid_before = pid;
  pid = id;
  qid = 3 - id;
  p = &players[id - 1];
  q = &players[2 - id];
  return pid_before;
}

int *copy_moves()
{
  int *backup, i;
  backup = (int *)malloc_safe(sizeof(int) * (turn + 1));
  backup[0] = turn;
  for (i = 0; i < turn; i++)
  {
    backup[i + 1] = moves[i];
  }
  return backup;
}

void restore_moves(int *backup)
{
  int backup_turn, i;
  backup_turn = backup[0];
  for (i = 0; i < min_(turn, backup_turn); i++)
  {
    if (backup[i + 1] != moves[i])
    {
      break;
    }
  }
  undo_moves(turn - i);
  for (; i < backup_turn; i++)
  {
    set_p(backup[i + 1] > 0 ? 1 : 2);
    submit_move(abs(backup[i + 1]));
  }
}

int win_in_3(char id)
{
  Player *p, *q;
  p = &players[id - 1];
  q = &players[2 - id];
  return p->double_fours.length && (!q->fives.length || (q->fives.length == 1 && p->fours[q->fives.values[0]].length >= 2));
}

int win_within_3()
{
  return p->fives.length || q->fives.length >= 2 || win_in_3(pid);
}
