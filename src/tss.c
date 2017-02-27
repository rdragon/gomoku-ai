// THREAT SPACE SEARCH

// in the following we assume n = 5

// THEORY

// a four-threat for black (say) is a line of five squares of which four are filled with black stones, and the fifth square is empty
// the empty square we call the cost square of the threat
// the last played square of the other four squares we call the gain square
// the other three squares we call the base squares of the threat

// a straight-four for black is a line of six squares of which the middle four squares are filled with black stones, and the outer two squares are empty

// a three-threat for black is a line of squares such that it is possible to construct a straight-four by placing a black stone on one of the empty squares
// the collection of cost squares of a three-threat is the collection of empty squares such that if one of the squares in this collection contains a white stone, then the threat is blocked (no straight-four can be constructed by placing a black stone on an empty square)
// the gain square of a three-threat is the last played black square
// the rest of the black squares we call the base squares of the threat
// if there are exactly two cost squares, then there exist two empty squares, both not a cost square, such that if both are filled with a white stone, then the threat is blocked. these two squares we call the end squares of the threat

// possible patterns of a three-threat:
// X = black stone, . = empty square
// c = cost square, e = end square
//
//  ..XXX..   .XXX..   .X.XX.   .X.XX.X.   .XX.X.XX.
//  ec   ce   c   cc   c c  c   e c  c e   e  c c  e
//
// note that the squares adjacent to the pattern also matter. for example, the second pattern requires the square to the left to contain a white stone, if that square exists. likewise, there is only a three-threat of the third pattern on the board if the pattern cannot be extended to the fourth or fifth pattern

// a threat `t` depends on a threat `t'` if the gain square of `t'` is one of the base squares of `t`
// two threats are in conflict if the gain square or a cost square of one of the threats is equal to the gain square or a cost square or an end square of the other threat

// a threat sequence is a sequence of threats that can be construced one after the other, while we allow the opponent to respond to each threat by playing all the cost squares of the threat. we also require that at no time the opponent is able to construct a five (a line of five adjacent stones in his color)
// a possibly winning threat sequence is one such that after the final move of the opponent, we can construct a five
// note that the threats in a threat sequence are pairwise non-conflicting

// # IMPLEMENTATION

// each threat `t` we give an index `t->index` in such a way that if `t` depends on `t'`, then `t->index` is greater than `t'->index`
// the dependency graph of a threat `t` is a directed graph with root `t`, such that the children of each node `t'` are the threats on which `t'` depends
// the threat sequence ending in a threat `t` consists of all the threats in the dependency graph of `t`, ordered by ascending index
// we only look at such threat sequences. therefore, not every possibly winning threat sequence will be found (todo: add example)

// ## EXPLANATION OF THE SEARCH

// for each threat that can be constructed by playing one move, we execute procedure 1
// PROCEDURE 1. let `t` be some threat. first we check if the opponent cannot create a five. if he cannot, we let the opponent respond by playing all the cost squares of `t`. then we check if we can now win the game. if we can, then we start procedure 2 (see below). otherwise, or if procedure 2 returns negative, we repeat procedure 1 for each threat `t'` that depends on `t` and which can be constructed by playing one additional move
// if no possibly winning threat sequence is found after the recursive calls to procedure 1, we start a combination stage
// COMBINATION STAGE. during a combination stage we look at all the threats founds thus far and try to construct new threats `t` that depend on two or three known threats. for each such `t`, if the threat sequence ending in `t` is valid, we execute procedure 1 for `t`
// we repeatedly run combination stages until a possibly winning threat sequence is found, or no new threats are found during some combination stage

// at each three-threat in a possibly winning threat sequence, the opponent is not forced to respond by playing one of the cost squares of the threat. instead, he can start a four-threat sequence for himself. this sequence can block our winning threat sequence, or might even win the game. therefore, when a possibly winning threat sequence is found, we execute the following procedure to filter out as many non-winning threat sequences as possible
// PROCEDURE 2. suppose we have found a possibly winning threat sequence. we construct the threats in our sequence one at a time. at each four-threat we let the opponent respond by playing the cost square of the threat. at each three-threat we look at all the four-threat sequences of the opponent. if any of these sequences wins the game, or has a non-zero intersection with our threat sequence, we return negative. otherwise we let the opponent play all the cost sqaures of the three-threat and continue. if we reach the end of our threat sequence, we return positive

#include "shared.h"

typedef struct Threat
{
  int gv;  // gain vector
  int cvs[3];  // cost vectors
  int evs[2];  // end vectors
  int cv_count;  // cost vector count
  int ev_count;  // end vector count
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
} ThreatCollection;

static Threat **threats;  // array of all found threats
static int threats_size;  // size of `threats`
static int threat_count;  // number of found threats
static int done;  // whether we should stop the search
static int combination_stage;  // the number of combination stages used
static int *moves_before;  // the state of the game at the start of the computation
static List list;

static int create_threats(ThreatCollection *);

void file_tss_ini()
{
  list_ini(&list, 9);
}

void file_tss_cleanup()
{
  list_cleanup(&list);
}

// stores the indices of all threats in the dependency graph of `t` in ascending order in `list`
// if `update_conflict_board` is set then it also keeps track of all the gain and cost and end vectors of the threats in the dependency graph, and stores this information in `conflict_board`
static void load_dependency_graph(Threat *t, int update_conflict_board)
{
  int k;
  list_ordered_add(&list, t->index);
  if (update_conflict_board == 1)
  {
    conflict_board[t->gv] = 1;
    for (k = 0; k < t->cv_count; k++)
    {
      conflict_board[t->cvs[k]] = 1;
    }
    for (k = 0; k < t->ev_count; k++)
    {
      conflict_board[t->evs[k]] = 2;
    }
  }
  for (k = 0; k < t->parent_count; k++)
  {
    if (!list_contains(&list, t->parent_threats[k]->index))
    {
      load_dependency_graph(t->parent_threats[k], update_conflict_board);
    }
  }
}

// writes the information about the possibly winning threat sequence in `list` to the results struct
static void handle_wts(int v)
{
  int k, l;
  Threat *t;
  result.threat_count = list.length;
  result.threats = (ResultThreat *)malloc_safe(sizeof(ResultThreat) * result.threat_count);
  for (k = 0; k < list.length; k++)
  {
    t = threats[list.values[k]];
    result.threats[k].gv = t->gv;
    result.threats[k].cv_count = t->cv_count;
    for (l = 0; l < t->cv_count; l++)
    {
      result.threats[k].cvs[l] = t->cvs[l];
    }
  }
  result.win_vec = v;
  result.combination_stage = combination_stage;
  result.success = 1;
  result.table = 0;
}

// checks whether the threat sequence in `list` is a possibly winning threat sequence
// `v` is the move that should win the game after the threat sequence is finished
static int check_wts()
{
  int k, v, i, u, success, *backup;
  Threat *t;
  for (u = v0; u < v1; u++)
  {
    win_board[u] = board[u];
  }
  for (k = 0; k < list.length; k++)
  {
    t = threats[list.values[k]];
    if (t->ev_count)
    {
      win_board[t->evs[0]] = 1;
      win_board[t->evs[1]] = 1;
    }
  }
  backup = copy_moves();
  for (i = 0; i < p->fives.length; i++)
  {
    v = p->fives.values[i];
    win_board[v] = 1;
    restore_moves(moves_before);
    success = 1;
    for (k = 0; k < list.length; k++)
    {
      t = threats[list.values[k]];
      submit_move(t->gv);
      if (t->cv_count >= 2)
      {
        if (tss_counter(pid) || out_of_time)
        {
          success = 0;
          break;
        }
      }
      submit_moves(t->cv_count, t->cvs);
    }
    win_board[v] = 0;
    if (success || out_of_time)
    {
      break;
    }
  }
  if (out_of_time)
  {
    done = 1;
  }
  else if (success)
  {
    handle_wts(v);
    done = 1;
  }
  restore_moves(backup);
  free(backup);
  return done;
}

// checks whether `t` is the final threat in a winning threat sequence
// otherwise tries to create other threats that depend on `t`
static void handle_new_threat(Threat *t)
{
  ThreatCollection col;
  submit_move(t->gv);
  if (!win_in_3(pid))
  {
    submit_moves(t->cv_count, t->cvs);
    if (p->fives.length)
    {
      list_clear(&list);
      load_dependency_graph(t, 0);
      check_wts();
    }
    else if (q->fives.length <= 1)
    {
      col.threat_count = 1;
      col.threats[0] = t;
      create_threats(&col);
    }
    undo_moves(t->cv_count);
  }
  undo_move();
}

// creates a new threat that depends on all threats in `col`
static void create_threat(ThreatCollection *col, int gv, int cv_count, int ev_count, ...)
{
  va_list ap;
  int k;
  Threat *t;
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
  t->cv_count = cv_count;
  t->ev_count = ev_count;
  va_start(ap, ev_count);
  for (k = 0; k < cv_count; k++)
  {
    t->cvs[k] = va_arg(ap, int);
  }
  for (k = 0; k < ev_count; k++)
  {
    t->evs[k] = va_arg(ap, int);
  }
  va_end(ap);
  handle_new_threat(t);
}

// creates new threats with gain vector `v` in the direction `d` that depend on all the threats in `col`
// it does not look at the threats in `col` to determine the dependency, but instead uses the values `rightmin` and `leftmin`
// it makes sure the new threats have at least one base vector of at most `rightmin` places to the right (in the direction of `d`) and at least one base vector of at most `leftmin` places to the left (in the direction of `-d`)
static int create_threats_at(ThreatCollection *col, int v, int d, int rightmin, int leftmin)
{
  int a, b, x, y, z;
  int left, right, ll, rr;
  if (q->fives.length && q->fives.values[0] != v)
  {
    return 0;
  }
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
  if (a + y < rightmin || b + z < leftmin)
  {
    return 0;
  }
  if (x + y == n)
  {
    create_threat(col, v, 1, 0, right);
  }
  if (x + z == n && !done)
  {
    create_threat(col, v, 1, 0, left);
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
    if (a + y < rightmin || b + z < leftmin)
    {
      return 0;
    }
    if (x + y == n && x + z == n)
    {
      create_threat(col, v, 2, 2, left, right, ll, rr);
    }
    else if (x + y == n)
    {
      create_threat(col, v, 3, 0, right, left, rr);
    }
    else if (x + z == n)
    {
      create_threat(col, v, 3, 0, left, right, ll);
    }
  }
  return done;
}

// creates new threats that have a gain vector of the form v + d * k with k >= 1 that depend on all the threats in `col`
// it does not look at the threats in `col` to determine the dependency, but instead it uses the values `rightmin` and `leftmin`
// it makes sure the new threats have at least one base vector of at most `rightmin` places to the right (in the direction of `d`) and at least one base vector of at most `leftmin` places to the left (in the direction of `-d`)
static int create_threats_tail(ThreatCollection *col, int v, int d, int rightmin, int leftmin)
{
  int broken;
  broken = 0;
  while (leftmin + 1 < n)
  {
    v += d;
    leftmin++;
    rightmin--;
    if (board[v] == 3 - pid)
    {
      break;
    }
    if (!board[v])
    {
      if (create_threats_at(col, v, d, rightmin, leftmin))
      {
        return 1;
      }
      if (broken)
      {
        break;
      }
      broken = 1;
    }
  }
  return 0;
}

// creates new threats that depend on all the threats in col
static int create_threats(ThreatCollection *col)
{
  int leftmin, rightmin, k, x, v;
  if (is_out_of_time())
  {
    done = 1;
    return 1;
  }
  if (!col->threat_count)
  {
    for (v = v0; v < v1; v++)
    {
      if (board[v] || !nearby[v])
      {
        continue;
      }
      if (create_threats_at(col, v, 1, 0, 0) ||
        create_threats_at(col, v, w, 0, 0) ||
        create_threats_at(col, v, se, 0, 0) ||
        create_threats_at(col, v, ne, 0, 0))
      {
        return 1;
      }
    }
  }
  else if (col->threat_count == 1)
  {
    v = col->threats[0]->gv;
    if (create_threats_tail(col, v, 1, 0, 0) ||
      create_threats_tail(col, v, -1, 0, 0) ||
      create_threats_tail(col, v, w, 0, 0) ||
      create_threats_tail(col, v, -w, 0, 0) ||
      create_threats_tail(col, v, se, 0, 0) ||
      create_threats_tail(col, v, nw, 0, 0) ||
      create_threats_tail(col, v, ne, 0, 0) ||
      create_threats_tail(col, v, sw, 0, 0))
    {
      return 1;
    }
  }
  else
  {
    v = col->threats[0]->gv;
    leftmin = 0;
    rightmin = 0;
    for (k = 1; k < col->threat_count; k++)
    {
      x = (col->threats[k]->gv - v) / col->d;
      leftmin = max_(leftmin, -x);
      rightmin = max_(rightmin, x);
    }
    return create_threats_tail(col, v, col->d, rightmin, leftmin)
      || create_threats_tail(col, v, -col->d, leftmin, rightmin);
  }
  return 0;
}

// checks whether a threat in the dependency graph of `t` is in conflict with a threat in `list` (this is done indirectly by using `conflict_board`)
static int conflicting_dependency_graph(Threat *t)
{
  int k;
  list_ordered_add(&list, t->index);
  if (conflict_board[t->gv])
  {
    return 1;
  }
  for (k = 0; k < t->cv_count; k++)
  {
    if (conflict_board[t->cvs[k]])
    {
      return 1;
    }
  }
  for (k = 0; k < t->ev_count; k++)
  {
    if (conflict_board[t->evs[k]] == 1)
    {
      return 1;
    }
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
  load_dependency_graph(t1, 1);  // add all threats in the dependency graph of `t1` to `list`, and updates `conflict_board`
  // check whether `t2` is a subsequence of `t1`
  if (list_contains(&list, t2->index))
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
  list_clear(&list);
  for (k = 0; k < col->threat_count; k++)
  {
    load_dependency_graph(col->threats[k], 0);
  }
  for (k = 0; k < list.length; k++)
  {
    t = threats[list.values[k]];
    submit_move(t->gv);
    if (p->fives.length || win_in_3(pid))
    {
      return 0;
    }
    submit_moves(t->cv_count, t->cvs);
    if (p->fives.length)
    {
      list.length = k + 1;
      check_wts();
      return 0;
    }
    else if (q->fives.length >= 2)
    {
      return 0;
    }
  }
  return 1;
}

// tries to make new threats by combining two or three threats
static int combine(int min_index, int max_index, ThreatCollection *col)
{
  Threat *t;
  int k, *backup;
  for (k = max_index; k >= min_index; k--)
  {
    t = threats[k];
    if (add_threat(col, t))
    {
      if (col->threat_count >= 2)
      {
        backup = copy_moves();
        if (do_all_moves(col))
        {
          create_threats(col);
        }
        restore_moves(backup);
        free(backup);
      }
      if (!done && col->threat_count < 3)
      {
        combine(0, k - 1, col);
      }
      col->threat_count--;
      if (done)
      {
        return 1;
      }
    }
  }
  return 0;
}

static void cleanup()
{
  int k;
  for (k = 0; k < threat_count; k++)
  {
    free(threats[k]);
  }
  free(threats);
  free(moves_before);
}

// searches for a possibly winning threat sequence. returns whether successful. additional information will be written to `result`
// assumes there is no winner within 2 moves
int tss(char id)
{
  ThreatCollection col;
  int threat_count_before, prev_threat_count;
  char pid_before;
  if (active_player->only_fours || win_in_3(id))
  {
    return tss_fours(id);
  }
  pid_before = set_p(id);
  threat_count = 0;
  done = 0;
  threats = 0;
  threats_size = 0;
  combination_stage = 0;
  moves_before = copy_moves();
  result.success = 0;
  result.only_fours = 0;
  col.threat_count = 0;
  prev_threat_count = 0;
  if (!create_threats(&col) && allow_combinations)
  {
    do
    {
      threat_count_before = threat_count;
      combination_stage++;
      if (combine(prev_threat_count, threat_count - 1, &col))
        break;
      prev_threat_count = threat_count_before;
    } while (threat_count_before != threat_count);
  }
  cleanup();
  set_p(pid_before);
  //temp print_("tss stats: %d threats, %d stages", threat_count, combination_stage);
  return result.success;
}

// returns whether player `id` has a possibly winning threat sequence after the game has settled into a "quiet" position, i.e., a position where `get_simple_move` returns null
// additional information will be written to `result`
int tss_quiescence(char id)
{
  int v, ans;
  if (winner)
  {
    result.success = 0;
    return winner == id;
  }
  v = get_simple_move();
  if (v)
  {
    submit_move(v);
    ans = tss_quiescence(id);
    undo_move();
    return ans;
  }
  else
  {
    return tss(id);
  }
}

static void handle_tss_results(float *scores)
{
  ResultThreat t;
  int i, j;
  if (result.success)
  {
    for (i = 0; i < result.threat_count; i++)
    {
      t = result.threats[i];
      scores[t.gv] = max_(scores[t.gv], -i * 2);
      for (j = 0; j < t.cv_count; j++)
      {
        scores[t.cvs[j]] = max_(scores[t.cvs[j]], -i * 2 - 1);
      }
    }
    free(result.threats);
  }
}

int is_safe_move(int v)
{
  int safe;
  submit_move(v);
  safe = !tss_quiescence(pid);
  undo_move();
  if (result.success)
  {
    free(result.threats);
  }
  return safe;
}

// returns a safe move for player `pid`, or -1 if all moves are safe
// assumes there is no win within 2 moves
static int find_safe_move(char *safeties)
{
  int v, u, ans;
  float *scores, best;
  if (!tss(qid))
  {
    return -1;
  }
  scores = (float *)malloc_safe(size * sizeof(float));
  for (v = v0; v < v1; v++)
  {
    safeties[v] = board[v] ? 0 : 2;
    scores[v] = nearby[v] ? -size : -FLT_MAX;
  }
  handle_tss_results(scores);
  u = 0;
  ans = 0;
  while (1)
  {
    best = -FLT_MAX;
    for (v = v0; v < v1; v++)
    {
      if (safeties[v] == 2 && scores[v] > best)
      {
        best = scores[v];
        u = v;
      }
    }
    if (best == -FLT_MAX)
    {
      break;
    }
    submit_move(u);
    safeties[u] = !tss_quiescence(pid);
    undo_move();
    if (out_of_time)
    {
      break;
    }
    if (safeties[u])
    {
      ans = u;
      break;
    }
    handle_tss_results(scores);
  }
  free(scores);
  return ans;
}

// assumes `get_simple_move()` returns 0
static int is_attack(int v)
{
  int attack;
  char *safeties;
  submit_move(v);
  attack = 0;
  if (tss(pid))
  {
    free(result.threats);
  }
  else if (!out_of_time)
  {
    if (q->fives.length)
    {
      attack = !is_safe_move(q->fives.values[0]);
    }
    else
    {
      safeties = (char *)malloc_safe(sizeof(char) * size);
      attack = !find_safe_move(safeties);
      free(safeties);
    }
  }
  undo_move();
  return out_of_time ? 0 : attack;
}

// assumes `get_simple_move()` returns 0
static int find_attack()
{
  int i, v, u;
  List moves;
  list_ini(&moves, empty_squares);
  for (v = v0; v < v1; v++)
  {
    if (!board[v] && nearby[v])
    {
      list_ordered_add_custom(&moves, v, compare_scores);
    }
  }
  u = 0;
  for (i = 0; i < moves.length; i++)
  {
    v = moves.values[i];
    if (is_attack(v))
    {
      u = v;
      break;
    }
    else if (out_of_time)
    {
      break;
    }
  }
  list_cleanup(&moves);
  return u;
}

int ai_tss()
{
  int v, len, unsafe_win;
  float time_tss_fours, time_tss, time_find_safe_move, time_alpha_beta, time_find_attack, time_table;
  char *safeties;
  v = ai_basic(0);
  if (v)
  {
    return v;
  }
  if (p->next_threat && p->result.only_fours)
  {
    if (p->next_threat == p->result.threat_count)
    {
      fail_("winning threat sequence unexepectedly ended");
    }
    if (verbose)
    {
      print_("creating the next threat in the winning threat sequence");
    }
    return p->result.threats[p->next_threat++].gv;
  }
  // else if (p->next_threat)
  // {
  //   t = &p->result.threats[p->next_threat - 1];
  //   for (k = 0; k < t->cv_count; k++)
  //   {
  //     if (board[t->cvs[k]])
  //     {
  //       break;
  //     }
  //   }
  //   if (k == t->cv_count)
  //   {
  //     v = ai_basic(1);
  //     if (!v)
  //     {
  //       fail_("winning threat sequence got interrupted!");
  //     }
  //     return v;
  //   }
  //   else
  //   {
  //     if (p->next_threat == p->result.threat_count)
  //     {
  //       fail_("winning threat sequence unexepectedly ended");
  //     }
  //     if (verbose)
  //     {
  //       print_("creating the next threat in the winning threat sequence");
  //     }
  //     return p->result.threats[p->next_threat++].gv;
  //   }
  // }
  v = ai_basic(1);
  if (v)
  {
    return v;
  }
  set_time_limit(time_left() * 0.75f);
  unsafe_win = 0;
  if (p->use_table)
  {
    unsafe_win = tss_fours_unsafe(pid);
  }
  else
  {
    tss_fours(pid);
  }
  time_tss_fours = get_elapsed_time();
  if (!result.success && !out_of_time && !p->only_fours)
  {
    tss(pid);
  }
  time_tss = get_elapsed_time();
  if (p->use_table && !result.success && unsafe_win && !out_of_time)
  {
    table_fours(pid);
  }
  time_table = get_elapsed_time();
  if (result.success)
  {
    if (verbose)
    {
      len = result.threat_count * 2 + 1;
      if (result.table)
      {
        print_("found a winning strategy consisting of %d moves. creating the first threat", len);
      }
      else
      {
        print_("found a");
        if (result.only_fours)
        {
          printf(" winning four-threat");
        }
        else
        {
          printf(" possibly winning threat");
        }
        if (result.combination_stage != 1)
        {
          printf(" sequence of length %d. used %d combination stages. creating the first threat", len, result.combination_stage);
        }
        else
        {
          printf(" sequence of length %d. used one combination stage. creating the first threat", len);
        }
      }
    }
    found_wts = 1;
    p->next_threat = 1;
    free(p->result.threats);
    p->result = result;
    v = result.threats[0].gv;
  }
  if (out_of_time && verbose)
  {
    print_("threat space search aborted: out of time");
  }
  if (!v && p->play_safe_move)
  {
    set_time_limit(time_left() * 0.75f);
    safeties = (char *)malloc_safe(sizeof(char) * size);
    safe_move = find_safe_move(safeties);
    if (safe_move == -1)
    {
      safe_move = 0;
    }
    else if (!safe_move && !out_of_time && verbose)
    {
      print_("no safe move available");
    }
    if (out_of_time && verbose)
    {
      print_("find_safe_move aborted: out of time");
    }
  }
  else
  {
    safeties = 0;
  }
  time_find_safe_move = get_elapsed_time();
  if (!v && 0)  //temp
  {
    set_time_limit(time_left() / 2);
    v = find_attack();
    if (v)
    {
      if (verbose)
      {
        print_("starting an attack");
      }
      found_wts = 1;
    }
    else if (out_of_time && verbose)
    {
      print_("find_attack aborted: out of time");
    }
  }
  time_find_attack = get_elapsed_time();
  if (!v)
  {
    set_time_limit(time_left());
    v = iterative_deepening(safeties);
  }
  time_alpha_beta = get_elapsed_time();
  if (!v)
  {
    v = random_empty_nearby_square();
    if (verbose)
    {
      print_("playing a random move");
    }
  }
  print_time(time_table, "table");
  print_time(time_tss_fours, "tss_fours");
  print_time(time_tss + time_tss_fours, "tss");
  print_time(time_find_safe_move, "find_safe_move");
  print_time(time_find_attack, "find_attack");
  print_time(time_alpha_beta, "alpha_beta");
  free(safeties);
  return v;
}
