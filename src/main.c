#include "shared.h"

static int black_arg;
static int white_arg;
static int block_arg;

static void test_block_configuration()
{
  int v, hit;
  v = random_empty_square();
  board[v] = 1;
  empty_squares--;
  update_nearby(v, 1);
  while (empty_squares)
  {
    hit = 0;
    for (v = v0; v < v1; v++)
    {
      if (!board[v] && nearby[v])
      {
        board[v] = 1;
        empty_squares--;
        update_nearby(v, 1);
        hit = 1;
      }
    }
    if (!hit)
    {
      break;
    }
  }
  if (empty_squares)
  {
    if (verbose)
    {
      printf("due to the configuration of the blocks on the board the alpha-beta search will be slower\n");
    }
    for (v = v0; v < v1; v++)
    {
      nearby[v] = 1;
      if (board[v] == 1)
      {
        board[v] = 0;
        empty_squares++;
      }
    }
  }
  else
  {
    for (v = v0; v < v1; v++)
    {
      if (board[v] == 1)
      {
        board[v] = 0;
        empty_squares++;
        update_nearby(v, -1);
      }
    }
  }
}

// adds the stones specified in the command line arguments to the board
void load_custom_board()
{
  int i, k;
  while ((i = parser_read_next_int(&block_arg)) != INT_MIN)
  {
    if (!board[to_v(i)])
    {
      board[to_v(i)] = 3;
      empty_squares--;
    }
  }
  for (k = 0; k < random_blocks; k++)
  {
    if (empty_squares)
    {
      board[random_empty_square()] = 3;
      empty_squares--;
    }
  }
  if (!empty_squares)
  {
    winner = draw;
  }
  else if (empty_squares < (w - 2) * (h - 2))
  {
    test_block_configuration();
  }
  while ((i = parser_read_next_int(&black_arg)) != INT_MIN)
  {
    if (!board[to_v(i)])
    {
      set_p(black_id);
      submit_move(to_v(i));
    }
  }
  while ((i = parser_read_next_int(&white_arg)) != INT_MIN)
  {
    if (!board[to_v(i)])
    {
      set_p(white_id);
      submit_move(to_v(i));
    }
  }
  if (random_moves)
  {
    set_p(white_starts ? white_id : black_id);
    for (k = 0; k < random_moves; k++)
    {
      if (winner)
      {
        break;
      }
      submit_move(random_empty_nearby_square());
    }
  }
  turn = 0;
  list_clear(&actions);
  if (json_client && show_board)
  {
    send_board();
  }
}

static void read_command_line_arguments()
{
  int battle;
  void **p_ai, **q_ai;
  set_p(1);
  default_ai = ai_tss;
  p->get_next_move = brain ? ai_alpha_beta : human;
  q->get_next_move = default_ai;
  p->heuristic = line_heur;
  q->heuristic = line_heur;
  p->alpha_beta = alpha_beta;
  q->alpha_beta = alpha_beta;
  p_ai = (void **)&p->get_next_move;
  q_ai = (void **)&q->get_next_move;
  while (!parser_done())
  {
    battle = 0;
    parser_read_help("-h", "--help", "show this help message");
    parser_read_int("-w", "--width", &w, 15, 1, 100, "set the width and height of the playing field");
    parser_read_int("-H", "--height", &h, 0, 0, 100, "specify a different height for the playing field");
    parser_read_int("-n", "--stones-to-win", &n, 5, 1, 100, "set the required minimum number of stones in a row to win");
    parser_read_bool("-q", "--quiet", &verbose, 1, "no verbose logging");
    parser_read_bool("-vq", "--very-quiet", &slightly_verbose, 1, "no slightly-verbose logging");
    parser_read_bool("-uv", "--ultra-verbose", &ultra_verbose, 0, "enable ultra-verbose logging");
    parser_read_bool("-sc", "--swap-colors", &swap_colors, 0, "player 2 is the black player in the first game");
    parser_read_bool("-fc", "--fixed-colors", &fixed_colors, 0, "the players do not swap colors after each game");
    parser_read_bool("-ms", "--manual-steps", &manual_steps, 0, "pause after each move in an ai-vs-ai game");
    parser_read_bool("-hb", "--hide-board", &show_board, !brain, "hide the playing field");
    parser_read_int("-rm", "--random-moves", &random_moves, 0, 0, INT_MAX, "specify the number of initial forced random moves");
    parser_read_int("-rb", "--random-blocks", &random_blocks, 0, 0, INT_MAX, "specify the number of initial random blocks");
    parser_read_int_array("--black", 0, &black_arg, "specify initial black stones");
    parser_read_int_array("--white", 0, &white_arg, "specify initial white stones");
    parser_read_int_array("--block", 0, &block_arg, "specify initial blocks");
    parser_read_bool("-ws", "--white-starts", &white_starts, 0, "the white player has the first move");
    parser_read_int_array("-pb", "--playback", &playback_arg, "specify the first moves to execute");
    parser_read_float2("-tl", "--time-limit", &p->time_limit, &q->time_limit, 0.5, 0, FLT_MAX, "set the turn time limit");
    parser_read_pointer2("-hm", "--human", p_ai, q_ai, (void *)human, "set ai to a human player");
    parser_read_pointer2("-ai", "--computer", p_ai, q_ai, (void *)default_ai, "set ai to the default ai");
    parser_read_pointer2("-tss", "--threat-space-search", p_ai, q_ai, (void *)ai_tss, "set ai to the threat space search ai");
    parser_read_pointer2("-ab", "--alpha-beta", p_ai, q_ai, (void *)ai_alpha_beta, "set ai to the alpha-beta ai");
    parser_read_pointer2("-pns", "--proof-number-search", p_ai, q_ai, (void *)ai_pns, "set ai to the proof-number search ai");
    parser_read_pointer2("-rd", "--random", p_ai, q_ai, (void *)ai_random, "the ai plays random moves");
    parser_read_int2("-fd", "--fixed-depth", &p->fixed_depth, &q->fixed_depth, 0, 0, INT_MAX, "specify a fixed depth for the alpha-beta search. time limit is ignored when a fixed depth is set");
    parser_read_bool2("-um", "--unsafe-moves", &p->play_safe_move, &q->play_safe_move, 1, "do not search for safe moves");
    parser_read_bool2("-nt", "--no-tracking", &p->track_board_value, &q->track_board_value, 1, "parameter used by alpha-beta");
    parser_read_int("-mml", "--max-mask-length", &max_mask_length, 10, 1, 15, "parameter used by alpha-beta");
    parser_read_float2("-ag", "--aggressiveness", &p->aggressiveness, &q->aggressiveness, 0, -1, 1, "determines the aggressiveness of the alpha-beta search");
    parser_read_bool2("-of", "--only-fours", &p->only_fours, &q->only_fours, 0, "only consider four-threat sequences");
    parser_read_bool2("-tb", "--table", &p->use_table, &q->use_table, 0, "temporary");
    parser_read_bool("-nc", "--no-combinations", &allow_combinations, 1, "skip the combination stages of the threat space search");
    parser_read_bool("-nh", "--no-halt-on-wts", &halt_on_wts, !brain, "do not switch to manual steps when a winning threat sequence is found");
    parser_read_bool("-swd", "--satisfied-with-draw", &satisfied_with_draw, 0, "the proof-number search ai is also satisfied with a draw instead of with a win only");
    parser_read_bool("-jc", "--json-client", &json_client, 0, "interact with a json client instead of the command line");
    parser_read_bool("-nsc", "--no-sanity-checks", &sanity_checks, 1, "no sanity checks");
    parser_read_bool("-as", "--auto-start", &auto_start, 0, "automatically start a new game when a game has ended");
    parser_read_bool("-b", "--battle", &battle, 0, "shortcut for --auto-start --hide-board --no-halt-on-wts --quiet --very-quiet");
    parser_read_bool("-peb", "--print-every-board", &print_every_board, 0, "print the state of the game after every move");
    parser_read_int("-s", "--seed", &initial_seed, -1, -1, INT_MAX, "initial seed for the random number generator");
    parser_read_bool("-hs", "--human-supervisor", &human_supervisor, 0, "have veto rights over the moves of the ai (for debugging)");
    if (battle)
    {
      auto_start = 1;
      show_board = 0;
      slightly_verbose = 0;
      verbose = 0;
      halt_on_wts = 0;
    }
  }
  if (!h)
  {
    h = w;
  }
  w += 2;
  h += 2;
  if (json_client)
  {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
  }
  else if (brain)
  {
    setvbuf(stdout, NULL, _IONBF, 0);
  }
  if (playback_arg && random_moves)
  {
    fail_("cannot combine playback with random moves");
  }
}

static char **load_arguments_from_file(int *count_in)
{
  FILE *f;
  int size, count, i, c;
  char buf[99], **values;
  values = 0;
  count = 0;
  f = fopen(file, "r");
  if (f)
  {
    size = 0;
    i = 0;
    do
    {
      c = fgetc(f);
      if (c == EOF || isspace(c))
      {
        if (i)
        {
          buf[i++] = '\0';
          if (count == size)
          {
            size = size ? size * 2 : 9;
            values = (char **)realloc_safe(values, sizeof(char *) * size);
          }
          values[count] = (char *)malloc_safe(sizeof(char) * i);
          strcpy(values[count], buf);
          count++;
        }
        i = 0;
      }
      else if (i < 98)
      {
        buf[i++] = (char)c;
      }
    } while (c != EOF);
    fclose(f);
  }
  *count_in = count;
  return values;
  return 0;
}

static void free_arguments(int count, char **values)
{
  int i;
  for (i = 0; i < count; i++)
  {
    free(values[i]);
  }
  free(values);
}

int main(int count, char **values)
{
  if (brain)
  {
    values = load_arguments_from_file(&count);
  }
  else
  {
    values = &values[1];
    count--;
  }
  parser_ini(count, values, "\nmany arguments have player 1 and player 2 counterparts by appending '1' or '2'. for example, to set player 2 to the default ai, use -ai2\n\nexample: gomoku-ai -w 4 -n 3 -ab1 -tss2 --block 5 10\n");
  while (!exit_now)
  {
    read_command_line_arguments();
    if (parser_displayed_help_message())
    {
      break;
    }
    run_game();
  }
  if (brain)
  {
    free_arguments(count, values);
  }
  return 0;
}
