// alpha-beta with iterative deepening

#include "shared.h"

const float winscore = 1e9;
const float drawscore = 0;

int safe_move;
static int depth;  // depth of the current alpha-beta search
static int best_move;  // best move found so far, according to the alpha-beta search (it is updated at the moment a new best move is found)
static List sorted_moves;

int compare_scores(int v, int u)
{
  return active_player->scores[v] >= active_player->scores[u] ? -1 : 1;
}

static int compare_scores_ext(int v, int u)
{
  if (v == best_move)
  {
    return  -1;
  }
  else if (u == best_move)
  {
    return 1;
  }
  else
  {
    return active_player->scores[v] >= active_player->scores[u] ? -1 : 1;
  }
}

static void sort_moves()
{
  int v;
  list_clear(&sorted_moves);
  for (v = v0; v < v1; v++)
  {
    if (!board[v])
    {
      list_ordered_add_custom(&sorted_moves, v, compare_scores_ext);
    }
  }
}

float alpha_beta(float alpha, float beta, int depth_left, int first_call)
{
  float score;
  int k, alpha_move, v;
  List *moves, _moves;
  if (is_out_of_time())
  {
    return 0;
  }
  if (winner == 3 - pid)
  {
    return truncate(-winscore * empty_squares, alpha, beta);
  }
  else if (winner == draw)
  {
    return truncate(drawscore * empty_squares, alpha, beta);
  }
  else if (p->fives.length)
  {
    return truncate(winscore * (empty_squares - 1), alpha, beta);
  }
  else if (q->fives.length >= 2)
  {
    return truncate(-winscore * (empty_squares - 2), alpha, beta);
  }
  else if (win_in_3(pid))
  {
    return truncate(winscore * (empty_squares - 3), alpha, beta);
  }
  else if (q->fives.length == 1)
  {
    v = q->fives.values[0];
    active_player->scores[v]++;
    submit_move(v);
    score = -active_player->alpha_beta(-beta, -alpha, depth_left, 0);
    undo_move();
    return score;
  }
  else if (!depth_left)
  {
    return truncate(active_player->heuristic(), alpha, beta);
  }
  alpha_move = 0;
  if (!first_call && depth_left > 1)
  {
    moves = &_moves;
    list_ini(moves, empty_squares);
    for (k = 0; k < sorted_moves.length; k++)
    {
      v = sorted_moves.values[k];
      if (!board[v] && nearby[v])
      {
        if (moves->length && compare_scores(list_peak(moves), v) < 0)
        {
          list_add(moves, v);
        }
        else
        {
          list_ordered_add_custom(moves, v, compare_scores);
        }
      }
    }
  }
  else
  {
    moves = &sorted_moves;
  }
  for (k = 0; k < moves->length; k++)
  {
    v = moves->values[k];
    if (moves == &sorted_moves && (board[v] || !nearby[v]))
    {
      continue;
    }
    submit_move(v);
    score = -active_player->alpha_beta(-beta, -alpha, depth_left - 1, 0);
    undo_move();
    if (out_of_time)
    {
      if (moves == &_moves)
      {
        list_cleanup(moves);
      }
      return -FLT_MAX;
    }
    if (score >= beta)
    {
      active_player->scores[v]++;
      if (moves == &_moves)
      {
        list_cleanup(moves);
      }
      return beta;
    }
    if (score > alpha)
    {
      alpha = score;
      alpha_move = v;
      if (first_call && v != best_move)
      {
        best_move = v;
      }
    }
  }
  if (alpha_move)
  {
    active_player->scores[alpha_move]++;
  }
  if (moves == &_moves)
  {
    list_cleanup(moves);
  }
  return alpha;
}

// the iterative deepening procedure, or a fixed depth search if `active_player->fixed_depth > 0`
// if `final_move` is set then the log messages reflect the fact that the move returned by this function is the move `active_player` is going to make
// assumes `!p->fives.length`
int iterative_deepening(char *safeties)
{
  float score, ans;
  int v, safe_move_depth;
  for (v = v0; v < v1; v++)
  {
    active_player->scores[v] /= 1e9;
  }
  if (active_player->track_board_value)
  {
    start_tracking_board_value();
  }
  if (active_player->fixed_depth)
  {
    set_time_limit(1e9);
  }
  depth = 1;
  best_move = 0;
  safe_move_depth = 0;
  score = 0;
  list_ini(&sorted_moves, empty_squares);
  while (depth <= empty_squares)
  {
    sort_moves();
    ans = active_player->alpha_beta(-FLT_MAX, FLT_MAX, depth, 1);
    if (out_of_time)
    {
      break;
    }
    score = ans;
    if (active_player->play_safe_move && safe_move)
    {
      if (score >= winscore)
      {
        safeties[best_move] = 1;
      }
      else if (safeties[best_move] == 2)
      {
        safeties[best_move] = is_safe_move(best_move);
      }
      if (safeties[best_move])
      {
        safe_move = best_move;
        safe_move_depth = depth;
      }
    }
    depth++;
    if (score >= winscore || score <= -winscore || (active_player->fixed_depth && depth > active_player->fixed_depth))
    {
      break;
    }
  }
  depth--;
  list_cleanup(&sorted_moves);
  if (track_board_value)
  {
    stop_tracking_board_value();
  }
  if (active_player->play_safe_move && safe_move)
  {
    v = safe_move;
  }
  else
  {
    v = best_move;
  }
  if (depth >= 1)
  {
    if (score >= winscore)
    {
      found_wts = 1;
    }
    if (verbose)
    {
      score = fabs(score) <= 0.05 ? 0 : score;
      if (depth == empty_squares)
      {
        print_("alpha-beta of depth %d (full depth) returned a score of %.1f", depth, score);
      }
      else
      {
        print_("alpha-beta of depth %d returned a score of %.1f", depth, score);
      }
      if (score >= winscore)
      {
        print_("win in %d", empty_squares - (int)(score / winscore));
      }
      else if (v == best_move && active_player->play_safe_move && safe_move)
      {
        print_("playing the returned move (a safe move)");
      }
      else if (v == best_move || safe_move_depth == depth)
      {
        print_("playing the returned move");
      }
      else if (safe_move_depth)
      {
        print_("playing the move of depth %d instead (a safe move)", safe_move_depth);
      }
      else
      {
        print_("playing a safe move instead");
      }
    }
    return v;
  }
  else
  {
    if (verbose)
    {
      print_("alpha-beta failed: out of time");
    }
    return 0;
  }
}

int ai_alpha_beta()
{
  int v;
  v = ai_basic(1);
  if (v)
  {
    return v;
  }
  set_time_limit(time_left());
  v = iterative_deepening(0);
  if (!v)
  {
    if (verbose)
    {
      print_("playing a random move");
    }
    v = random_empty_nearby_square();
  }
  return v;
}
