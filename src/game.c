#include "shared.h"

int n;  // n-on-a-row
int w, h;  // dimensions of the playing field
int size;  // `size = w * h`
int v0, v1;
int se, ne, sw, nw;

char *board;  // the playing field. an array of length `size`. we picture the indices 0, w-1, and size-1 as respectivly the top-left, top-right and bottom-right corners of the playing field
char pid;  // id of the player to move (equal to 1 or 2)
char qid;
char winner;  // id of the winner, or `draw` for a draw. a value of 0 indicates that the game is still running
int empty_squares;  // number of empty squares
int turn;  // number of the current turn, starting at zero
int *moves;  // history of all moves in chronological order (an array of length `turn`)
char *nearby;  // `nearby[v]` equals the number of stones in the 5x5 subfield with center `v`

char black_id;  // id of the black player
char white_id;  // id of the white player
Player players[2];  // the two players
Player *p; // the player corresponding to `pid`
Player *q; // the player corresponding to `3 - pid`
Player *active_player;  // the player who tries to compute his next move
char active_id;  // id of `active_player`. in general different from `pid` (as every ai may temporarily change the state of the board)
int active_turn; // the number of the turn of `active_player`
int games_played; // the number of finished games
float start_of_turn;  // time stamp of start of turn
float end_of_turn;  // time stamp before turn should end
float stop_time;  // time stamp of end of time limit (as set by `set_time_limit`)
int out_of_time;  // whether we ran out of time at a call to `check_out_of_time`
char *conflict_board;  // used by threat space search to detect conflicting threats
char *win_board;  // used by threat space search in the detection of counter four-threat sequences
int playback_arg;
int(*default_ai)();
int exit_now;  // if set then we are exiting the program
int first_print;
int print_every_board;
TssResult result;
int found_wts;
List actions;

int slightly_verbose;  // slightly verbose logging
int verbose;  // verbose logging
int ultra_verbose;  // ultra-verbose logging
int manual_steps;  // whether after each move in an ai-vs-ai game we wait for a key from the client
int halt_on_wts;  // if set then we switch to manual steps when a winning threat sequence is found
int show_board;  // whether we should show the board
int json_client;  // whether we interact with a json client
int swap_colors;  // if set then player 2 starts the first game
int fixed_colors;  // if not set then each game the players swap colors
int satisfied_with_draw;  // whether the proof-number search ai is satisfied with a draw (instead of only with a win)
int allow_combinations;  // if set then we allow combination stages in threat space search
int random_moves;  // if `random_moves > 0` then the game starts with `random_moves` forced random moves (used to counter the determinism in ai-vs-ai games)
int random_blocks;  // the number of random placed blocks at the start of the game
int white_starts;  // if set then the white player has the first move
int sanity_checks;  // if set then we run a number of sanity checks during execution
int auto_start;  // if set then we automatically start a new game when a game has ended
int initial_seed;
int human_supervisor;

static int playback_active;
static void start_brain_loop();

// should be called after the width and the height of the playing field are known
void initialize_globals()
{
  int k;
  static int seed;
  Player *r;
  size = w * h;
  board = (char *)calloc_safe(size, sizeof(char));
  nearby = (char *)calloc_safe(size, sizeof(char));
  conflict_board = (char *)calloc_safe(size, sizeof(char));
  win_board = (char *)calloc_safe(size, sizeof(char));
  moves = (int *)malloc_safe(size * sizeof(int));
  winner = 0;
  turn = 0;
  if (!games_played)
  {
    black_id = swap_colors ? 2 : 1;
  }
  else if (!fixed_colors)
  {
    black_id = white_id;
  }
  white_id = 3 - black_id;
  se = w + 1;
  ne = 1 - w;
  sw = -ne;
  nw = -se;
  v0 = se;
  v1 = size + nw;
  empty_squares = (w - 2) * (h - 2);
  list_ini(&actions, 99);
  for (k = 0; k < w; k++)
  {
    board[k] = 3;
    board[k + w * (h - 1)] = 3;
  }
  for (k = 1; k < h - 1; k++)
  {
    board[k * w] = 3;
    board[w - 1 + w * k] = 3;
  }
  for (k = 0; k < 2; k++)
  {
    r = &players[k];
    r->scores = (float *)calloc_safe(size, sizeof(float));
    r->fours = (List *)calloc_safe(size, sizeof(List));
    r->five_count = (char *)calloc_safe(size, sizeof(char));
    list_ini(&r->fives, 9);
    list_ini(&r->double_fours, 9);
    list_ini(&r->fl, 9);
    r->fl_count = (char *)calloc_safe(size, sizeof(char));
    r->next_threat = 0;
  }
  file_line_heur_ini();
  file_tss_fours_ini();
  file_tss_ini();
  if (!games_played)
  {
    if (initial_seed == -1)
    {
      seed = (int)time(0);
    }
    else
    {
      seed = initial_seed;
    }
  }
  else if (games_played % 2 == 0 || fixed_colors)
  {
    seed = rand();
  }
  srand((unsigned int)seed);
}

void free_globals()
{
  int k, i;
  Player *r;
  free(board);
  free(nearby);
  free(conflict_board);
  free(win_board);
  free(moves);
  list_cleanup(&actions);
  for (k = 0; k < 2; k++)
  {
    r = &players[k];
    for (i = 0; i < size; i++)
    {
      list_cleanup(&r->fours[i]);
    }
    free(r->scores);
    free(r->fours);
    free(r->five_count);
    list_cleanup(&r->fives);
    list_cleanup(&r->double_fours);
    list_cleanup(&r->fl);
    free(r->fl_count);
    free(r->result.threats);
    r->result.threats = 0;
  }
  file_line_heur_cleanup();
  file_tss_fours_cleanup();
  file_tss_cleanup();
}

// executes a single turn
static void play_one_turn()
{
  int v, i;
  active_player = &players[pid - 1];
  active_id = pid;
  first_print = 1;
  safe_move = 0;
  found_wts = 0;
  v = 0;
  i = parser_read_next_int(&playback_arg);
  playback_active = i != INT_MIN;
  if (!playback_active || (human_supervisor && p->get_next_move != human))
  {
    active_turn = turn;
    start_of_turn = get_time();
    end_of_turn = start_of_turn + p->time_limit;
    get_elapsed_time();
    v = p->get_next_move();
    if (human_supervisor)
    {
      print_("move = (%d, %d)", v % w - 1, v / w - 1);
      if (!playback_active)
      {
        if (json_client)
        {
          printf("\n");
          first_print = 1;
        }
        v = human();
      }
    }
  }
  if (playback_active)
  {
    if (verbose)
    {
      print_("playing the next move in the playback");
    }
    v = to_v(i);
    if (human_supervisor)
    {
      print_("move = (%d, %d)", v % w - 1, v / w - 1);
    }
  }
  if (!first_print)
  {
    printf("\n");
  }
  if (found_wts && !playback_active)
  {
    on_wts();
  }
  if (exit_now)
  {
    return;
  }
  submit_move(v);
  if (show_board)
  {
    if (ultra_verbose)
    {
      print_game_state(stdout);
    }
    if (json_client)
    {
      send_stone(v);
    }
    else
    {
      print_board();
    }
  }
}

// starts the game loop
void run_game()
{
  int backup;
  if (brain)
  {
    start_brain_loop();
    exit_now = 1;
    return;
  }
  initialize_globals();
  if (json_client)
  {
    send_new_game_msg();
  }
  load_custom_board();
  set_p(white_starts ^ (random_moves % 2) ? white_id : black_id);
  if (slightly_verbose)
  {
    printf("starting game %d. player %d (black) versus player %d (white)\n", games_played + 1, black_id, white_id);
  }
  if (verbose && !games_played && !json_client)
  {
    printf("note: works best with a fixed-width font having a width : height ratio of 2:3\n");
  }
  if (!json_client && show_board)
  {
    print_board();
  }
  while (!winner)
  {
    play_one_turn();
    if (print_every_board)
    {
      backup = turn;
      turn = 0;
      print_game_state(stdout);
      turn = backup;
    }
    if (((manual_steps && no_human_player()) || playback_active) && !winner && !exit_now)
    {
      if (verbose && !json_client)
      {
        printf("press enter to start the next turn\n");
      }
      //wait_for_newline();
    }
    if (exit_now)
    {
      free_globals();
      return;
    }
  }
  if (winner == draw)
  {
    players[black_id - 1].black_draws++;
    players[white_id - 1].white_draws++;
  }
  else if (winner == black_id)
  {
    players[winner - 1].black_wins++;
  }
  else
  {
    players[winner - 1].white_wins++;
  }
  games_played++;
  if (winner == draw)
  {
    printf("game %d has finished in a draw after %d turns", games_played, turn);
  }
  else
  {
    printf("game %d is won by player %d after %d turns", games_played, winner, turn);
  }
  if (games_played > 1)
  {
    printf(". results of player 1: %d+%d / %d+%d / %d+%d", players[0].black_wins, players[0].white_wins, players[1].white_wins, players[1].black_wins, players[0].black_draws, players[0].white_draws);
    if (games_played == 2 || verbose)
    {
      printf(" (wins as black + wins as white / losses as black + losses as white / draws)");
    }
  }
  printf("\n");
  if (slightly_verbose || show_board)
  {
    print_game_state(stdout);
  }
  if (!auto_start)
  {
    if (slightly_verbose)
    {
      printf("press enter to start a new game (or type \"q\" to exit)\n");
    }
    wait_for_newline();
  }
  free_globals();
}

int no_human_player()
{
  return players[0].get_next_move != human && players[1].get_next_move != human;
}

static void get_next_move()
{
  int v;
  set_p(1);
  play_one_turn();
  v = moves[turn - 1];
  printf("%d,%d\n", v % w - 1, v / w - 1);
}

static void start_brain_loop()
{
  char buf[999], *buf_y;
  int x, y, id;
  while (1)
  {
    if (!fgets(buf, sizeof buf, stdin))
    {
      fail_("`fgets` returned zero");
    }
    if (strstr(buf, "START") == buf)
    {
      w = atoi(buf + 6) + 2;
      h = w;
      initialize_globals();
      printf("OK\n");
    }
    else if (strstr(buf, "INFO") == buf)
    {
      if (strstr(buf + 5, "timeout_turn"))
      {
        players[0].time_limit = (float)atoi(buf + 18) / 1000;
      }
    }
    else if (strstr(buf, "TAKEBACK") == buf)
    {
      undo_move();
    }
    else if (strstr(buf, "BEGIN") == buf || strstr(buf, "TURN") == buf)
    {
      if (strstr(buf, "TURN") == buf)
      {
        set_p(2);
        submit_move(1 + atoi(buf + 5) + w * (1 + atoi(&strstr(buf, ",")[1])));
      }
      get_next_move();
    }
    else if (strstr(buf, "END") == buf || strstr(buf, "RESTART") == buf)
    {
      free_globals();
      if (strstr(buf, "END") == buf)
      {
        return;
      }
      initialize_globals();
      printf("OK\n");
    }
    else if (strstr(buf, "BOARD") == buf)
    {
      while (1)
      {
        if (!fgets(buf, sizeof buf, stdin))
        {
          fail_("`fgets` returned zero");
        }
        if (strstr(buf, "DONE") == buf)
        {
          get_next_move();
          break;
        }
        else
        {
          buf_y = &strstr(buf, ",")[1];
          x = atoi(buf) + 1;
          y = atoi(buf_y) + 1;
          id = atoi(&strstr(buf_y, ",")[1]);
          set_p(id);
          submit_move(x + w * y);
        }
      }
    }
  }
}
