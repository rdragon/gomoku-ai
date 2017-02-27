// THREAT SPACE SEARCH (of only four-threats)

// in the following we assume n = 5

// # THEORY

// a four-threat for black (say) is a line of 5 squares of which 4 are filled with black stones, and the fifth square is empty
// the empty square we call the cost square of the four-threat
// the last played square of the other four squares we call the gain square
// the other three squares we call the base squares of the threat

// a threat `t` depends on a threat `t'` if the gain square of `t'` is one of the base squares of `t`
// two four-threats are in conflict if the gain or cost square of one of the threats is equal to the gain or cost square of the other threat

// a four-threat sequence is a sequence of four-threats that can be construced one after the other, while the opponent is forced to respond to each threat by playing the cost square of the threat (he will lose if he plays any other move)
// a winning four-threat sequence is one such that after the final move of the opponent, we can construct a five (a line of five adjacent stones in our color)
// note that the threats in a four-threat sequence are pairwise non-conflicting

// # IMPLEMENTATION

// each threat `t` we give an index `t->index` in such a way that if `t` depends on `t'`, then `t->index` is greater than `t'->index`
// the dependency graph of a threat `t` is a directed graph with root `t`, such that the children of each node `t'` are the threats on which `t'` depends
// the threat sequence ending in a threat `t` consists of all the threats in the dependency graph of `t`, ordered by ascending index
// we only look at such threat sequences. therefore, not every winning four-threat sequence will be found (todo: add example)

// ## EXPLANATION OF THE SEARCH

// for each four-threat that can be constructed by playing one move, we execute procedure 1
// PROCEDURE 1. let `t` be some four-threat. first we check if the opponent cannot create a five. if he cannot, we let the opponent respond by playing the cost square of `t`. then we check if we can now win the game. if we can, then we return the winning threat sequence ending in threat `t`. otherwise, we repeat procedure 1 for each four-threat `t'` that depends on `t` and which can be constructed by playing one additional move
// if no winning threat sequence is found after the recursive calls to procedure 1, we start a combination stage
// COMBINATION STAGE. during a combination stage we look at all the threats founds thus far and try to construct new threats `t` that depend on two or three known threats. for each such `t`, if the threat sequence ending in `t` is valid, we execute procedure 1 for `t`
// we repeatedly run combination stages until a winning threat sequence is found, or no new threats are found during some combination stage

#include "shared.h"

typedef struct Threat
{
  int gv, cv;  // gain vector, cost vector
  struct Threat *parent_threats[4];  // the parent threats, ie, the threats on which this threat depends
  char parent_count;  // the parent threat count
  int index;  // the index of this threat in `threats`
} Threat;

// a collection of threats. used to store threats which combined may result in a new threat
typedef struct ThreatCollection
{
  Threat *threats[4];
  int threat_count;
  int d;
  int safe;
} ThreatCollection;

static Threat **threats;  // array of all found threats
static int threat_count;  // number of found threats
static int threats_size;  // size of `threats`
static int done;  // whether we should stop the search
static int combination_stage;
static List list;
static int counter;
static int counter_success;
static int ignore_counters;
static int unsafe_win;

static void create_threats(ThreatCollection *);

void file_tss_fours_ini()
{
  list_ini(&list, 9);
}

void file_tss_fours_cleanup()
{
  list_cleanup(&list);
}

// stores the indices of all threats in the dependency graph of `t` in ascending order in `list`
// if `update_conflict_board` is set then it also keeps track of all the gain and cost vectors of the threats in the dependency graph, and stores this information in `conflict_board`
static void load_dependency_graph(Threat *t, int update_conflict_board)
{
  int k;
  list_ordered_add(&list, t->index);
  if (update_conflict_board == 1)
  {
    conflict_board[t->gv] = 1;
    conflict_board[t->cv] = 1;
  }
  for (k = 0; k < t->parent_count; k++)
  {
    if (!list_ordered_contains(&list, t->parent_threats[k]->index))
    {
      load_dependency_graph(t->parent_threats[k], update_conflict_board);
    }
  }
}

// writes the information about the winning threat sequence in `list` to the result struct
static void handle_wts(Threat *t, int safe)
{
  int k, v;
  if (counter)
  {
    done = 1;
    counter_success = 1;
    return;
  }
  if (!safe)
  {
    unsafe_win = 1;
    ignore_counters = 0;
    return;
  }
  if (t)
  {
    list_clear(&list);
    load_dependency_graph(t, 0);
  }
  result.success = 1;
  result.threat_count = list.length + 1;
  result.threats = (ResultThreat *)malloc_safe(sizeof(ResultThreat) * result.threat_count);
  for (k = 0; k < list.length; k++)
  {
    result.threats[k].gv = threats[list.values[k]]->gv;
    result.threats[k].cv_count = 1;
    result.threats[k].cvs[0] = threats[list.values[k]]->cv;
  }
  v = q->fives.length ? q->fives.values[0] : p->double_fours.values[0];
  result.threats[list.length].gv = v;
  result.threats[list.length].cv_count = 1;
  result.threats[list.length].cvs[0] = p->fours[v].values[0];
  result.win_vec = p->fours[v].values[1];
  result.combination_stage = combination_stage;
  result.table = 0;
  done = 1;
}

// checks whether the threat sequence ending in `t` is winning
// if not, it tries to create other threats that depend on `t`
// resembles procedure 1 from the comments at the top of this file
static void handle_new_threat(Threat *t, int safe)
{
  ThreatCollection col;
  if (!safe && !ignore_counters)
  {
    return;
  }
  submit_move(t->gv);
  if (p->fours[t->cv].length >= 2)
  {
    if (ignore_counters)
    {
      safe = 0;
    }
    else
    {
      undo_move();
      return;
    }
  }
  submit_move(t->cv);
  if (win_in_3(pid) || (ignore_counters && p->double_fours.length))
  {
    handle_wts(t, safe && win_in_3(pid));
  }
  else
  {
    col.threat_count = 1;
    col.threats[0] = t;
    col.safe = safe;
    create_threats(&col);
  }
  undo_moves(2);
}

// creates a new threat with gain vector `gv` and cost vector `cv` that depends on all threats in `col`
static void create_threat(ThreatCollection *col, int gv, int cv)
{
  int k, safe;
  Threat *t;
  safe = col->safe;
  if (q->fives.length && gv != q->fives.values[0])
  {
    if (ignore_counters)
    {
      safe = 0;
    }
    else
    {
      return;
    }
  }
  if (ignore_counters && q->five_count[cv])
  {
    return;
  }
  if (counter && (win_board[gv] || win_board[cv]))
  {
    handle_wts(0, safe);
    return;
  }
  t = (Threat *)calloc_safe(1, sizeof(Threat));
  if (threat_count >= threats_size)
  {
    threats_size = max_(threats_size * 2, 8);
    threats = (Threat **)realloc_safe(threats, threats_size * sizeof(Threat *));
  }
  t->index = threat_count++;
  threats[t->index] = t;
  t->parent_count = col->threat_count;
  for (k = 0; k < col->threat_count; k++)
  {
    t->parent_threats[k] = col->threats[k];
  }
  t->gv = gv;
  t->cv = cv;
  handle_new_threat(t, safe);
}

static void create_threats_1(ThreatCollection *col)
{
  int gv, u, v, v2, d, d2, i, k, l, dir, sandwich, ok;
  int dirs[4] = { 1, w, 1 + w, 1 - w };
  gv = col->threats[0]->gv;
  for (dir = 0; dir < 4; dir++)
  {
    d = dirs[dir];
    sandwich = 0;
    for (i = 0; i < 2; i++)  // loop over `d` and `-d`
    {
      v = gv + d;
      for (k = 1; k < n; k++)  // v = gv + k * d
      {
        if (!board[v] && p->fours[v].length)
        {
          v2 = p->fours[v].values[0];
          d2 = get_d(gv, v2);
          if (d2 == d)
          {
            u = v + d;
            for (l = k; l < n; l++)
            {
              if (u != v2 && board[u] != pid)
              {
                break;
              }
              u += d;
            }
            ok = l < n;
          }
          else if (d2 == -d && !sandwich)
          {
            ok = 1;
            sandwich = 1;
          }
          else
          {
            ok = 0;
          }
          if (ok)
          {
            create_threat(col, v, v2);
            create_threat(col, v2, v);
          }
          break;
        }
        else if (board[v] != pid)
        {
          break;
        }
        v += d;
      }
      d *= -1;
    }
  }
}

static int check_ok(int gv, int v, int v2)
{
  int d, d2, u, k;
  d = get_d(gv, v);
  d2 = get_d(gv, v2);
  if (d == -d2)
  {
    return 1;
  }
  u = gv + d;
  for (k = 0; k < n; k++)
  {
    if (u != v && u != v2 && board[u] != pid)
    {
      break;
    }
    u += d;
  }
  return k < n;
}

static void create_threats_2(ThreatCollection *col)
{
  int gv, u, v, v2, d, d2, i, k, l, sandwich, ok;
  gv = col->threats[0]->gv;
  d = col->d;
  sandwich = 0;
  for (i = 0; i < 2; i++)  // loop over `d` and `-d`
  {
    v = gv + d;
    for (k = 1; k < n; k++)  // v = gv + k * d
    {
      if (!board[v] && p->fours[v].length)
      {
        v2 = p->fours[v].values[0];
        d2 = get_d(gv, v2);
        if (d2 == d)
        {
          u = v + d;
          for (l = k; l < n; l++)
          {
            if (u != v2 && board[u] != pid)
            {
              break;
            }
            u += d;
          }
          ok = l < n;
        }
        else if (d2 == -d && !sandwich)
        {
          ok = 1;
          sandwich = 1;
        }
        else
        {
          ok = 0;
        }
        if (ok)
        {
          for (k = 1; k < col->threat_count; k++)
          {
            if (!check_ok(col->threats[k]->gv, v, v2))
            {
              ok = 0;
              break;
            }
          }
          if (ok)
          {
            create_threat(col, v, v2);
            create_threat(col, v2, v);
          }
        }
        break;
      }
      else if (board[v] != pid)
      {
        break;
      }
      v += d;
    }
    d *= -1;
  }
}

// creates new threats that depend on all the threats in `col`
static void create_threats(ThreatCollection *col)
{
  int v, u, i;
  if (is_out_of_time())
  {
    done = 1;
  }
  else if (!col->threat_count)
  {
    for (i = 0; i < p->fl.length; i++)
    {
      v = p->fl.values[i];
      u = p->fours[v].values[0];
      if (v < u)
      {
        create_threat(col, u, v);
        create_threat(col, v, u);
      }
    }
  }
  else if (col->threat_count == 1)
  {
    create_threats_1(col);
  }
  else
  {
    create_threats_2(col);
  }
}

// checks whether a threat in the dependency graph of `t` is in conflict with a threat in `list` (this is done indirectly by using `conflict_board`)
static int conflicting_dependency_graph(Threat *t)
{
  int k;
  list_ordered_add(&list, t->index);
  if (conflict_board[t->gv] || conflict_board[t->cv])
  {
    return 1;
  }
  for (k = 0; k < t->parent_count; k++)
  {
    if (!list_ordered_contains(&list, t->parent_threats[k]->index))
    {
      if (conflicting_dependency_graph(t->parent_threats[k]))
      {
        return 1;
      }
    }
  }
  return 0;
}

// checks whether it may be possible to play both the threat sequences of `t1` and `t2`, and that one is not a subsequence of the other
// this is done by checking whether the dependency graphs of `t1` and `t2` do not contain conflicting threats
static int possible_valid_combination(Threat *t1, Threat *t2)
{
  Threat *swap;
  // after this swap we know for sure that the threat sequence of `t1` is not a subsequence of the threat sequence of `t2`, as `t1` is not in the dependency tree of `t2`, which follows from the fact that `t1` is found after `t2`
  if (t2->index > t1->index)
  {
    swap = t2;
    t2 = t1;
    t1 = swap;
  }
  board_clear(conflict_board);
  list_clear(&list);
  load_dependency_graph(t1, 1);  // add all threats in the dependency graph of `t1` to `list`, and all corresponding gain and cost vectors to `conflict_board`
  // check whether `t2` is a subsequence of `t1`
  if (list_ordered_contains(&list, t2->index))
  {
    return 0;
  }
  return !conflicting_dependency_graph(t2);
}

// tries to add `t` to `col` and returns whether this was successful
static int add_threat(ThreatCollection *col, Threat *t)
{
  int k, d, v;
  if (col->threat_count)
  {
    d = get_d(col->threats[0]->gv, t->gv);
    if (!d)
    {
      return 0;
    }
    if (col->threat_count > 1)
    {
      if (d != col->d && d != -col->d)
      {
        return 0;
      }
      for (k = 1; k < col->threat_count; k++)
      {
        if (!get_d(col->threats[k]->gv, t->gv))
        {
          return 0;
        }
      }
    }
    v = col->threats[0]->gv + d;
    while (v != t->gv)
    {
      if (board[v] && board[v] != pid)
      {
        return 0;
      }
      v += d;
    }
    for (k = 0; k < col->threat_count; k++)
    {
      if (!possible_valid_combination(col->threats[k], t))
      {
        return 0;
      }
    }
    if (col->threat_count == 1)
    {
      col->d = d;
    }
  }
  col->threats[col->threat_count++] = t;
  return 1;
}

// returns whether the combination of the threat sequences of the threats in `col` is a threat sequence
static int do_all_moves(ThreatCollection *col)
{
  int k;
  Threat *t;
  col->safe = 1;
  list_clear(&list);
  for (k = 0; k < col->threat_count; k++)
  {
    load_dependency_graph(col->threats[k], 0);
  }
  for (k = 0; k < list.length; k++)
  {
    t = threats[list.values[k]];
    if (ignore_counters && q->five_count[t->cv])
    {
      return 0;
    }
    submit_move(t->gv);
    if (p->fours[t->cv].length >= 2 || (p->fours[t->cv].length && k + 1 < list.length && p->fours[t->cv].values[0] != threats[list.values[k + 1]]->gv))
    {
      if (ignore_counters)
      {
        col->safe = 0;
      }
      else
      {
        return 0;
      }
    }
    submit_move(t->cv);
    if (win_in_3(pid) || (ignore_counters && p->double_fours.length))
    {
      list.length = k + 1;
      handle_wts(0, col->safe && win_in_3(pid));
      return 0;
    }
  }
  return 1;
}

// tries to make new threats by combining two or three threats
static void combine(int min_index, int max_index, ThreatCollection *col)
{
  Threat *t;
  int k, *moves_before;
  moves_before = copy_moves();
  for (k = max_index; k >= min_index; k--)
  {
    t = threats[k];
    if (add_threat(col, t))
    {
      if (col->threat_count >= 2)
      {
        if (do_all_moves(col))
        {
          create_threats(col);
        }
        restore_moves(moves_before);
      }
      if (!done && col->threat_count < 3)
      {
        combine(0, k - 1, col);
      }
      col->threat_count--;
      if (done)
      {
        break;
      }
    }
  }
  free(moves_before);
}

static void cleanup()
{
  int k;
  for (k = 0; k < threat_count; k++)
  {
    free(threats[k]);
  }
  free(threats);
}

// searches for a winning four-threat sequence. returns whether successful. additional information will be written to `result`
// assumes there is no winner within 2 moves
int tss_fours(char id)
{
  ThreatCollection col;
  int threat_count_before, prev_threat_count;
  char pid_before;
  pid_before = set_p(id);
  threat_count = 0;
  done = 0;
  threats = 0;
  threats_size = 0;
  combination_stage = 0;
  col.threat_count = 0;
  col.safe = 1;
  prev_threat_count = 0;
  if (counter)
  {
    counter_success = 0;
  }
  else
  {
    result.success = 0;
    result.only_fours = 1;
  }
  if (win_in_3(pid) || (ignore_counters && p->double_fours.length))
  {
    list_clear(&list);
    handle_wts(0, win_in_3(pid));
  }
  else
  {
    create_threats(&col);
    if (!done && allow_combinations)
    {
      do
      {
        threat_count_before = threat_count;
        combination_stage++;
        combine(prev_threat_count, threat_count - 1, &col);
        prev_threat_count = threat_count_before;
      } while (!done && threat_count != threat_count_before);
    }
  }
  cleanup();
  set_p(pid_before);
  return result.success;
}

int tss_fours_unsafe(char id)
{
  ignore_counters = 1;
  unsafe_win = 0;
  tss_fours(id);
  ignore_counters = 0;
  return unsafe_win;
}

int tss_counter(char id)
{
  counter = 1;
  tss_fours(id);
  counter = 0;
  return counter_success;
}
