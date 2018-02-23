
#ifndef PARSER_H_
#define PARSER_H_

#ifndef _REENTRANT
#define _REENTRANT

#include "shell.h"

int parse();

typedef struct tokenizer {
	char *str;
	char *pos;
} tokenizer;

#endif
#endif