// proof-number search

#include "shared.h"

// the children of a node `r` are `r->child`, `r->child->nxt`, `r->child->nxt->nxt`, ...
typedef struct Node
{
  int proof_number;  // proof number
  int disproof_number;  // disproof number
  int v;  // the move which leads to this node
  struct Node *parent;
  struct Node *child;  // first child
  struct Node *nxt;  // next child of parent
  struct Node *prev;  // previous child of parent
} Node;

static char root_id;  // id of the player corresponding to the root node
static int ans;  // whether the position is proven or disproven
static int returned_move; // the move returned by the search

static void update(Node *, char);

static int is_root(Node *r)
{
  return !r->parent;
}

static void free_nodes(Node *r)
{
  if (r)
  {
    free_nodes(r->child);
    free_nodes(r->nxt);
    free(r);
  }
}

// this is called after proving or disproving node `r`. it will update the parent node of `r` and remove `r` from the tree
// `id` is the id of the player to move at node `r`
// `val` is the value of node `r`. it indicates whether node `r` is proven or disproven
static void remove_node(Node *r, char id, int val)
{
  Node *s;
  free_nodes(r->child);
  r->child = 0;
  if (is_root(r))
  {
    ans = val;
    return;
  }
  if (val == (id != root_id))
  {
    if (is_root(r->parent))
    {
      returned_move = r->v;
    }
    remove_node(r->parent, 3 - id, val);
  }
  else
  {
    if (r->prev)
    {
      r->prev->nxt = r->nxt;
    }
    else
    {
      r->parent->child = r->nxt;
    }
    if (r->nxt)
    {
      r->nxt->prev = r->prev;
    }
    s = r->parent;
    free(r);
    update(s, 3 - id);
  }
}

// this is called when there is a change in a child of `r`
// `id` is the id of the player to move at node `r`
static void update(Node *r, char id)
{
  Node *s;
  if (!r->child)
  {
    remove_node(r, id, id != root_id);
    return;
  }
  if (id == root_id)
  {
    r->proof_number = INT_MAX;
    r->disproof_number = 0;
    s = r->child;
    while (s)
    {
      r->proof_number = min_(r->proof_number, s->proof_number);
      r->disproof_number += s->disproof_number;
      s = s->nxt;
    }
  }
  else
  {
    r->proof_number = 0;
    r->disproof_number = INT_MAX;
    s = r->child;
    while (s)
    {
      r->proof_number += s->proof_number;
      r->disproof_number = min_(r->disproof_number, s->disproof_number);
      s = s->nxt;
    }
  }
  if (r->parent)
  {
    update(r->parent, 3 - id);
  }
}

// creates the children of node `r`
static void extend_node(Node *r)
{
  int val;
  Node *s, *prev;
  int v;
  prev = 0;
  for (v = v0; v < v1; v++)
  {
    if (!board[v])
    {
      submit_move(v);
      if (winner)
      {
        val = winner == root_id || (winner == draw && satisfied_with_draw);
      }
      else
      {
        val = -1;
      }
      undo_move();
      if (val == (pid == root_id))
      {
        if (is_root(r))
        {
          returned_move = v;
        }
        remove_node(r, pid, val);
        return;
      }
      else if (val == -1)
      {
        s = (Node *)malloc_safe(sizeof(Node));
        if (prev)
        {
          prev->nxt = s;
        }
        else
        {
          r->child = s;
        }
        s->parent = r;
        s->child = 0;
        s->nxt = 0;
        s->prev = prev;
        s->v = v;
        s->proof_number = pid != root_id ? 1 : empty_squares - 1;
        s->disproof_number = pid == root_id ? 1 : empty_squares - 1;
        prev = s;
      }
    }
  }
  update(r, pid);
}

// extends the tree by calling `extend_node` on the leaf that is easiest to prove
static void extend_tree(Node *r)
{
  Node *s, *best;
  int v;
  if (!r->child)
  {
    extend_node(r);
    return;
  }
  best = r->child;
  s = best->nxt;
  if (pid == root_id)
  {
    while (s)
    {
      if (s->proof_number < best->proof_number)
      {
        best = s;
      }
      s = s->nxt;
    }
  }
  else
  {
    while (s)
    {
      if (s->disproof_number < best->disproof_number)
      {
        best = s;
      }
      s = s->nxt;
    }
  }
  v = best->v;
  submit_move(v);
  extend_tree(best);
  undo_move();
}

// executes proof number search. returns a valid move if the search was successful
int pns()
{
  Node r;
  r.proof_number = 1;
  r.disproof_number = 1;
  r.nxt = 0;
  r.prev = 0;
  r.child = 0;
  r.v = 0;
  r.parent = 0;
  root_id = pid;
  ans = -1;
  while (ans == -1)
  {
    extend_tree(&r);
    if (is_out_of_time())
    {
      return 0;
    }
  }
  return ans ? returned_move : 0;
}

int ai_pns()
{
  int v;
  v = ai_basic(1);
  if (v)
  {
    return v;
  }
  set_time_limit(time_left() * 3 / 4);
  v = pns();
  if (v)
  {
    if (verbose)
    {
      if (satisfied_with_draw)
      {
        print_("position is a proven draw or win according to proof-number search. playing a suitable move");
      }
      else
      {
        print_("position is a proven win according to proof-number search. playing a suitable move");
      }
    }
    if (!satisfied_with_draw)
    {
      found_wts = 1;
    }
    return v;
  }
  if (verbose)
  {
    if (out_of_time)
    {
      print_("proof-number search aborted: out of time");
    }
    else
    {
      if (satisfied_with_draw)
      {
        print_("position is a proven lose according to proof-number search");
      }
      else
      {
        print_("position is a proven draw or lose according to proof-number search");
      }
    }
  }
  set_time_limit(time_left());
  v = iterative_deepening(0);
  if (!v)
  {
    if (verbose)
    {
      print_("playing a random move");
    }
    return random_empty_nearby_square();
  }
  return v;
}
