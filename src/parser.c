// a command line arguments parser

#include "parser.h"

#define max_(a, b) ((b) > (a) ? (b) : (a))
#define fail_(s, ...) { fprintf(stderr, "error while reading command line arguments: "); fprintf(stderr, s, ##__VA_ARGS__); fprintf(stderr, "\n"); exit(EXIT_FAILURE); }

static int count; // number of arguments to be read
static char **arguments; // the arguments to be read
static int k;  // index of the next argument to be read
static int k_before;  // the value of `k` at the start of the current phase
static int max_length;  // the maximum value returned by `get_length` after the `compute_max_length` phase
static int helper_hit;  // whether an argument asking for help was read
static const int padding = 4;  // minimum number of spaces between an argument and the explanation of the argument
static const char *epilogue;  // a message to be displayed at the end of the help message
static int help_message_printed;  // whether the help message has been shown
static int set_defaults;  // the special phase in which each variable for which a default value is supplied is set to this default value
static int compute_max_length;  // the special phase in which `max_length` is computed
static int print_arguments;  // the special phase in which all arguments and their explanations are printed

void parser_ini(int count_in, char **arguments_in, const char *epilogue_in)
{
  count = count_in;
  arguments = arguments_in;
  epilogue = epilogue_in;
  k = -1;
}

int parser_displayed_help_message()
{
  return help_message_printed;
}

static void missing_arg_fail()
{
  fail_("expecting an argument following \"%s\", but none is given", arguments[k]);
}

// returns the length of the argument part of `print_argument_line`
static int get_length(const char *name1, const char *name2)
{
  if (name2)
  {
    return 1 + strlen(name1) + 2 + strlen(name2);
  }
  else
  {
    return 1 + strlen(name1);
  }
}

// prints a line with an argument and its explanation
static void print_argument_line(const char *name1, const char *name2, const char *desc)
{
  int i;
  if (name2)
  {
    printf(" %s, %s", name1, name2);
  }
  else
  {
    printf(" %s", name1);
  }
  for (i = get_length(name1, name2); i < max_length + padding; i++)
  {
    printf(" ");
  }
  printf("%s", desc);
}

// returns 1 if the k'th argument is equal to `a` or `b`, and 0 otherwise. except when `allow_suffix` is set, then a value of 1, 2, or 3 is returned, depending on whether the k'th argument is equal to `a` or `b` with a suffix of respectively '1', '2', or '' (no suffix). if the k'th argument is not equal to any of these values, a value of 0 is returned
static int get_hit(const char *a, const char *b, int allow_suffix)
{
  char *c, x;
  int hit;
  c = arguments[k];
  if (!strcmp(a, c))
  {
    return allow_suffix ? 3 : 1;
  }
  else if (b && !strcmp(b, c))
  {
    return allow_suffix ? 3 : 1;
  }
  else if (allow_suffix)
  {
    hit = c == strstr(c, a) && strlen(a) + 1 == strlen(c);
    if (!hit)
    {
      hit = b && c == strstr(c, b) && strlen(b) + 1 == strlen(c);
    }
    if (hit)
    {
      x = c[strlen(c) - 1];
      if (x == '1')
      {
        return 1;
      }
      else if (x == '2')
      {
        return 2;
      }
    }
  }
  return 0;
}

// returns whether the parsing has finished. if this is not the case, then it starts a new phase. in each phase (excluding the special phases) one argument (together with its possible sub-arguments) is read
int parser_done()
{
  if (k == -1)
  {
    k_before = k;
    k = 0;
    help_message_printed = 0;
    set_defaults = 1;
  }
  else if (set_defaults)
  {
    set_defaults = 0;
    if (!count)
    {
      k = -1;
      return 1;
    }
  }
  else if (helper_hit)
  {
    helper_hit = 0;
    compute_max_length = 1;
  }
  else if (compute_max_length)
  {
    compute_max_length = 0;
    print_arguments = 1;
  }
  else if (print_arguments)
  {
    if (epilogue)
    {
      printf("%s", epilogue);
    }
    print_arguments = 0;
    k = -1;
    help_message_printed = 1;
    return 1;
  }
  else if (k == count)
  {
    k = -1;
    return 1;
  }
  else if (k == k_before)
  {
    if (!strlen(arguments[k]))
    {
      k++;
      return parser_done();
    }
    else
    {
      fail_("unknown argument: \"%s\"", arguments[k]);
      return 1;
    }
  }
  else
  {
    k_before = k;
  }
  return 0;
}

void parser_read_int2(const char *name1, const char *name2, int *val1, int *val2, int default_val, int min, int max, const char *desc)
{
  long int x;
  char *end;
  int hit;
  if (set_defaults)
  {
    *val1 = default_val;
    if (val2)
    {
      *val2 = default_val;
    }
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf(" (default=%d)\n", default_val);
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  hit = get_hit(name1, name2, !!val2);
  if (hit)
  {
    if (k == count - 1)
    {
      missing_arg_fail();
      return;
    }
    k++;
    x = strtol(arguments[k], &end, 10);
    if (end == arguments[k] || end[0] != '\0')
    {
      fail_("expecting integer argument following \"%s\", but got \"%s\" instead", arguments[k - 1], arguments[k]);
    }
    if (x < min || x > max)
    {
      fail_("argument \"%d\" following \"%s\" is out of bounds (min=%d, max=%d)", (int)x, arguments[k - 1], min, max);
      return;
    }
    if (hit & 1)
    {
      *val1 = (int)x;
    }
    if (hit & 2)
    {
      *val2 = (int)x;
    }
    k++;
  }
}

void parser_read_float2(const char *name1, const char *name2, float *val1, float *val2, float default_val, float min, float max, const char *desc)
{
  float x;
  char *end;
  int hit;
  if (set_defaults)
  {
    *val1 = default_val;
    if (val2)
    {
      *val2 = default_val;
    }
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf(" (default=%f)\n", default_val);
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  hit = get_hit(name1, name2, !!val2);
  if (hit)
  {
    if (k == count - 1)
    {
      missing_arg_fail();
      return;
    }
    k++;
    x = (float)strtod(arguments[k], &end);
    if (end == arguments[k] || end[0] != '\0')
    {
      fail_("expecting float argument following \"%s\", but got \"%s\" instead", arguments[k - 1], arguments[k]);
    }
    if (x < min || x > max)
    {
      fail_("argument \"%f\" following \"%s\" is out of bounds (min=%f, max=%f)", x, arguments[k - 1], min, max);
      return;
    }
    if (hit & 1)
    {
      *val1 = x;
    }
    if (hit & 2)
    {
      *val2 = x;
    }
    k++;
  }
}

void parser_read_bool2(const char *name1, const char *name2, int *val1, int *val2, int default_val, const char *desc)
{
  int hit;
  if (set_defaults)
  {
    *val1 = default_val;
    if (val2)
    {
      *val2 = default_val;
    }
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf("\n");
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  hit = get_hit(name1, name2, !!val2);
  if (hit)
  {
    if (hit & 1)
    {
      *val1 = !default_val;
    }
    if (hit & 2)
    {
      *val2 = !default_val;
    }
    k++;
  }
}

void parser_read_pointer2(const char *name1, const char *name2, void **val1, void **val2, void *custom_val, const char *desc)
{
  int hit;
  if (set_defaults)
  {
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf("\n");
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  hit = get_hit(name1, name2, !!val2);
  if (hit)
  {
    if (hit & 1)
    {
      *val1 = custom_val;
    }
    if (hit & 2)
    {
      *val2 = custom_val;
    }
    k++;
  }
}

// reads an argument and sets `*val = custom_val` when successful
void parser_read_custom_int(const char *name1, const char *name2, int *val, int custom_val, const char *desc)
{
  if (set_defaults)
  {
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf("\n");
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  if (get_hit(name1, name2, 0))
  {
    *val = custom_val;
    k++;
  }
}

// reads an argument that requires an integer sub-argument between `min` and `max` (including). `*val` is set to `default_val` at the start of the parsing
void parser_read_int(const char *name1, const char *name2, int *val, int default_val, int min, int max, const char *desc)
{
  parser_read_int2(name1, name2, val, 0, default_val, min, max, desc);
}

// reads an argument that requires a float sub-argument between `min` and `max` (including). `*val` is set to `default_val` at the start of the parsing
void parser_read_float(const char *name1, const char *name2, float *val, float default_val, float min, float max, const char *desc)
{
  parser_read_float2(name1, name2, val, 0, default_val, min, max, desc);
}

// reads an argument and sets `*val = !default_val` when successful. `*val` is set to `default_val` at the start of the parsing
void parser_read_bool(const char *name1, const char *name2, int *val, int default_val, const char *desc)
{
  parser_read_bool2(name1, name2, val, 0, default_val, desc);
}

// reads an argument and sets `*val = custom_val` when successful
void parser_read_pointer(const char *name1, const char *name2, void **val, void *custom_val, const char *desc)
{
  parser_read_pointer2(name1, name2, val, 0, custom_val, desc);
}

// reads the special help argument
void parser_read_help(const char *name1, const char *name2, const char *desc)
{
  if (set_defaults)
  {
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf("\n");
    }
    return;
  }
  if (get_hit(name1, name2, 0))
  {
    helper_hit = 1;
  }
}

// returns the argument on position `*index` as an integer value, or `INT_MIN` if the conversion fails. increments `*index` when the conversion succeeds
int parser_read_next_int(int *index)
{
  char *end;
  long int x;
  if (!*index || *index == count || !strlen(arguments[*index]) || arguments[*index][0] == '-')
  {
    return INT_MIN;
  }
  x = strtol(arguments[*index], &end, 10);
  if (end == arguments[*index] || end[0] != '\0')
  {
    return INT_MIN;
  }
  (*index)++;
  return (int)x;
}

// reads an argument that requires a non-empty list of integer sub-arguments
void parser_read_int_array(const char *name1, const char *name2, int *val, const char *desc)
{
  if (set_defaults)
  {
    return;
  }
  if (compute_max_length)
  {
    if (desc)
    {
      max_length = max_(max_length, get_length(name1, name2));
    }
    return;
  }
  if (print_arguments)
  {
    if (desc)
    {
      print_argument_line(name1, name2, desc);
      printf("\n");
    }
    return;
  }
  if (k != k_before)
  {
    return;
  }
  if (get_hit(name1, name2, 0))
  {
    k++;
    *val = k;
    while (parser_read_next_int(&k) != INT_MIN) {}
    if (k == k_before + 1)
    {
      missing_arg_fail();
      return;
    }
  }
}
