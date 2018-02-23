
#ifndef PARSER_H_
#define PARSER_H_

#ifndef _REENTRANT
#define _REENTRANT

#include "shell.h"
#include "builtins.h"

int parse();

void free_all_jobs();

typedef struct tokenizer {
	char *str;
	char *pos;
} tokenizer;

#endif
#endif