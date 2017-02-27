#ifndef PARSER_H
#define PARSER_H

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>

void parser_ini(int, char **, const char *);
void parser_read_int(const char *, const char *, int *, int, int, int, const char *);
void parser_read_float(const char *, const char *, float *, float, float, float, const char *);
void parser_read_bool(const char *, const char *, int *, int, const char *);
void parser_read_pointer(const char *, const char *, void **, void *, const char *);
void parser_read_int2(const char *, const char *, int *, int *, int, int, int, const char *);
void parser_read_float2(const char *, const char *, float *, float *, float, float, float, const char *);
void parser_read_pointer2(const char *, const char *, void **, void **, void *, const char *);
void parser_read_bool2(const char *, const char *, int *, int *, int, const char *);
void parser_read_custom_int(const char *, const char *, int *, int, const char *);
void parser_read_help(const char *, const char *, const char *);
void parser_read_int_array(const char *, const char *, int *, const char *);
int parser_displayed_help_message();
int parser_read_next_int(int *);
int parser_done();

#endif
