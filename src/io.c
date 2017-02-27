#include "shared.h"

static char symbols[] = " X-/";  // console game symbols

static int is_quit_message(char *line)
{
  return !strcmp(line, "q\n") || !strcmp(line, "quit\n") || !strcmp(line, "exit\n");
}

// prints a representation of the current state of the board as a valid list of command line arguments which can be passed to the executable
// assumes that the turns alternate between black and white
void print_game_state(FILE *stream)
{
  int k, first, v, i, j, *backup;
  fprintf(stream, "-w %d", w - 2);
  if (w != h)
  {
    fprintf(stream, " -H %d", h - 2);
  }
  if (n != 5)
  {
    fprintf(stream, " -n %d", n);
  }
  if (white_starts ^ (random_moves % 2))
  {
    fprintf(stream, " -ws");
  }
  if (empty_squares != (w - 2) * (h - 2) - turn)
  {
    backup = copy_moves();
    undo_moves(turn);
    for (k = 1; k <= 3; k++)
    {
      first = 1;
      v = v0;
      for (i = 2; i < h; i++)
      {
        for (j = 2; j < w; j++)
        {
          if (board[v] == k)
          {
            if (first)
            {
              fprintf(stream, " --%s", k == black_id ? "black" : (k == 3 ? "block" : "white"));
              first = 0;
            }
            fprintf(stream, " %d", from_v(v));
          }
          v++;
        }
        v += 2;
      }
    }
    restore_moves(backup);
    free(backup);
  }
  if (turn)
  {
    fprintf(stream, " -pb");
    for (k = 0; k < turn; k++)
    {
      fprintf(stream, " %d", from_v(abs(moves[k])));
    }
  }
  fprintf(stream, "\n");
}

// sends the complete board to the json client
void send_board()
{
  int v, i, j;
  printf("{\"msg\": \"clear board\"}\n");
  v = v0;
  for (i = 2; i < h; i++)
  {
    for (j = 2; j < w; j++)
    {
      if (board[v])
      {
        send_stone(v);
      }
      v++;
    }
    v += 2;
  }
}

void send_stone(int v)
{
  int stone;
  if (black_id == 2 && board[v] != 3)
  {
    stone = 3 - board[v];
  }
  else
  {
    stone = board[v];
  }
  printf("{\"msg\": \"place stone\", \"pos\": %d, \"stone\": %d}\n", from_v(v), stone);
}

void send_new_game_msg()
{
  printf("{\"msg\": \"new game\", \"pars\": [%d, %d]}\n", w - 2, h - 2);
}

void send_msg(const char *msg)
{
  printf("{\"msg\": \"%s\"}\n", msg);
}

// prints the board. assumes a fixed-width font having a width : height ratio of 2:3
void print_board()
{
  int i, j, k, last_move, id;
  char c;
  int v;
  last_move = turn ? abs(moves[turn - 1]) : 0;
  printf("|\n");
  for (j = 2; j < w; j++)
  {
    printf("||||||");
  }
  printf("||||\n");
  for (i = 2; i < h; i++)
  {
    for (k = 0; k < 4; k++)
    {
      printf("||");
      v = v0 + w * (i - 2);
      for (j = 2; j < w; j++)
      {
        id = board[v];
        if (id == 1 || id == 2)
        {
          if (black_id == 1)
          {
            c = symbols[id];
          }
          else
          {
            c = symbols[3 - id];
          }
          if (k == 1 || k == 2 || v == last_move)
          {
            printf(" %c%c%c%c ", c, c, c, c);
          }
          else
          {
            printf("  %c%c  ", c, c);
          }
        }
        else if (id == 3)
        {
          if (k == 1 || k == 2)
          {
            printf("  %c%c  ", symbols[id], symbols[id]);
          }
          else
          {
            printf("      ");
          }
        }
        else
        {
          if (k == 1)
          {
            printf("  %c%c  ", 'a' + (i - 2), 'a' + (j - 2));
          }
          else
          {
            printf("      ");
          }
        }
        v++;
      }
      printf("||\n");
    }
  }
  for (j = 2; j < w; j++)
  {
    printf("||||||");
  }
  printf("||||\n|\n");
}

// executes a single turn of a human player
int human()
{
  char buf[9];
  char *end;
  int v;
  if (json_client)
  {
    send_msg("your turn");
  }
  else
  {
    if (slightly_verbose)
    {
      printf("it is your turn. please give some coordinates (for example, \"aa\")\n");
    }
  }
  v = 0;
  while (!v)
  {
    if (!fgets(buf, sizeof buf, stdin))
    {
      fail_("error while reading input. possibly encountered an unexpected end-of-file");
    }
    if (is_quit_message(buf))
    {
      exit_now = 1;
      return 0;
    }
    if (json_client)
    {
      v = (int)strtol(buf, &end, 10);
      if (v)
      {
        v = to_v_unsafe(v - 1);
      }
    }
    else
    {
      v = buf[1] - 'a' + 1 + w * (buf[0] - 'a' + 1);
    }
    if (v < 0 || v >= size || board[v])
    {
      v = 0;
    }
    if (!v)
    {
      printf("invalid input or illegal move. please try again\n");
    }
  }
  return v;
}

void wait_for_newline()
{
  char buf[99];
  if (fgets(buf, sizeof buf, stdin))
  {
    if (is_quit_message(buf))
    {
      exit_now = 1;
    }
    else if (!strcmp(buf, "finish\n") || !strcmp(buf, "f\n"))
    {
      manual_steps = 0;
      halt_on_wts = 0;
    }
    else if (!strcmp(buf, "human-supervisor\n") || !strcmp(buf, "hs\n"))
    {
      human_supervisor = 1;
    }
  }
}

int to_v(int i)
{
  int x, y;
  if (i < 0 || i >= (w - 2) * (h - 2))
  {
    fail_("input position %d is out of bounds", i);
  }
  x = i % (w - 2) + 1;
  y = i / (w - 2) + 1;
  return x + w * y;
}

int to_v_unsafe(int i)
{
  int x, y;
  x = i % (w - 2) + 1;
  y = i / (w - 2) + 1;
  return x + w * y;
}

int from_v(int v)
{
  int x, y;
  x = v % w - 1;
  y = v / w - 1;
  return x + (w - 2) * y;
}

void print_player_prefix()
{
  if (first_print)
  {
    first_print = 0;
    if (brain)
    {
      printf("MESSAGE ");
    }
    printf("[player %d, turn %d] ", active_id, turn + 1);
  }
  else
  {
    printf(". ");
  }
}
