#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include "list.h"
#include "parser.h"

#define min_(a, b) ((b) < (a) ? (b) : (a))
#define max_(a, b) ((b) > (a) ? (b) : (a))
#define fail_(s, ...) { fprintf(stderr, "fatal error in %s at line %d: ", __FILE__, __LINE__); fprintf(stderr, s, ##__VA_ARGS__); fprintf(stderr, "\n"); fail(); }
#define print_(s, ...) { print_player_prefix(); printf(s, ##__VA_ARGS__); }
#define draw 3

#ifndef brain
// adds support for communication with gomoku interfaces like piskvork (http://gomocup.org/piskvork/). the protocol is explained at http://petr.lastovicka.sweb.cz/protocl2en.htm
#define brain 0
#endif

#ifndef file
#define file ""
#endif

typedef unsigned long long ullong;
typedef unsigned int uint;

typedef struct ResultThreat
{
  int gv;  // gain vector
  int cvs[3];  // cost vectors
  int cv_count;  // cost vector count
} ResultThreat;

typedef struct TssResult
{
  int success;  // whether the search was successful
  int only_fours;  // whether the threat sequence consists of only four-threats
  ResultThreat *threats;  // the threats in the threat sequence
  int threat_count;  // the number of threats in the threat sequence
  int combination_stage;  // the number of combination stages in the threat sequence. in other words, the number of times the threat sequence combines independent threats to from a new threat
  int win_vec;
  int table;
} TssResult;

typedef struct Player
{
  float time_limit;  // the time (in seconds) the player is allowed to spend on each turn
  int(*get_next_move)();  // computes the next move for this player
  float(*heuristic)();  // the evaluation heuristic for alpha-beta
  float *scores;  // determines the order in which the moves are traversed by the alpha-beta search
  float(*alpha_beta)(float, float, int, int);  // the alpha-beta function to use
  int fixed_depth;  //  if `fixed_depth > 0` then the alpha-beta search is of depth `fixed_depth` and we do not use iterative deepening
  int track_board_value;
  float aggressiveness;
  List *fours;
  char *five_count;
  List fives;
  List double_fours;
  int play_safe_move;
  int only_fours;
  TssResult result;
  int next_threat;  // threat to create at the next turn
  int black_wins;
  int white_wins;
  int black_draws;
  int white_draws;
  int use_table;
  List fl;  // a list containing of every four threat at least the smaller of the gain vector and the cost vector
  char *fl_count;
} Player;

extern int max_mask_length;
extern int track_board_value;
extern float board_value;
extern float *board_values;
extern int w, h;
extern int size;
extern int ne, se, nw, sw;
extern int v0, v1;
extern int n;
extern int turn;
extern int active_turn;
extern int empty_squares;
extern int *moves;
extern char *board;
extern char winner;
extern char pid, qid;
extern int first_print;
extern Player *p;
extern Player *q;
extern char *nearby;
extern int verbose;
extern float start_of_turn;
extern float end_of_turn;
extern float stop_time;
extern int slightly_verbose;
extern int out_of_time;
extern int ultra_verbose;
extern int manual_steps;
extern int halt_on_wts;
extern int show_board;
extern int json_client;
extern int satisfied_with_draw;
extern int allow_combinations;
extern char *win_board;
extern char *conflict_board;
extern int safe_move;
extern int swap_colors;
extern Player players[];
extern Player *active_player;
extern char active_id;
extern const float winscore;
extern const float drawscore;
extern int(*default_ai)();
extern int random_moves;
extern int random_blocks;
extern const float winscore;
extern const float drawscore;
extern char black_id;
extern char white_id;
extern int auto_start;
extern int exit_now;
extern int white_starts;
extern int playback_arg;
extern int sanity_checks;
extern int games_played;
extern int draws;
extern int fixed_colors;
extern int print_every_board;
extern TssResult result;
extern int initial_seed;
extern int found_wts;
extern List actions;
extern int human_supervisor;

// main
void load_custom_board();

// game
void initialize_globals();
void free_globals();
int no_human_player();
void run_game();

// board
void board_clear(char *);
char set_p(char);
int tail_length(int, int, char);
void update_nearby(int, int);
void submit_move(int);
void submit_moves(int, int *);
void place_stone(int, char);
void undo_move();
void undo_moves(int);
int get_simple_move();
int do_forced_move_opt();
int random_empty_square();
int random_empty_nearby_square();
int not_nearby(int);
int *copy_moves();
void restore_moves(int *);
int compare_scores(int, int);
int win_in_3(char);
int win_within_3();

// io
void print_game_state(FILE *);
void send_board();
void send_stone(int);
void send_new_game_msg();
void send_msg(const char *);
void print_board();
int human();
void wait_for_newline();
int to_v(int);
int to_v_unsafe(int);
int from_v(int);
void print_player_prefix();

// rest
int ai_random();
int ai_basic(int);
void fail();
void *calloc_safe(size_t, size_t);
void *malloc_safe(size_t);
void *realloc_safe(void *, size_t);
float get_time();
float time_left();
void set_time_limit(float);
int is_out_of_time();
void on_wts();
float truncate(float, float, float);
int get_d(int, int);
float get_elapsed_time();
void print_time(float, const char *);

// alpha beta
float alpha_beta(float, float, int, int);
int iterative_deepening(char *);
int ai_alpha_beta();

// line heur
float line_heur();
void file_line_heur_ini();
void file_line_heur_cleanup();
void start_tracking_board_value();
void stop_tracking_board_value();
void update_board_value(int, int);
void revert_board_value(int);

// tss
int tss_fours(char);
int tss_fours_unsafe(char);
void file_tss_fours_ini();
void file_tss_fours_cleanup();
int is_safe_move(int);
int tss_counter(char);
int tss(char);
int tss_quiescence(char);
int ai_tss();
void file_tss_ini();
void file_tss_cleanup();

// table
int table_fours(char);
int table_tss();

// pns
int pns();
int ai_pns();

#endif
