#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "parser.h"

#define BUFFER_SIZE 4096
#define CONTINUE 1
#define EXIT 0
#define TRUE 1
#define FALSE 0
#define NUM_DELIMINATORS 3

char *command_delimintators[NUM_DELIMINATORS] = {"&" , ";", "\0"};
char *white_space_deliminator = " ";
char *PROMPT = "$ ";

typedef struct tokenizer {
	char *str;
	char *pos;
} tokenizer;

tokenizer *t;

int split_white_space(char **user_input, char ***tokenized_input)
{
	int buffer_mark = BUFFER_SIZE;

	(*tokenized_input) = malloc(sizeof(char*)*BUFFER_SIZE);
	if (*tokenized_input == NULL) { return EXIT; }

	int i = 0;
	(*tokenized_input)[i] = strtok(*user_input, " ");

	while ((*tokenized_input)[i] != NULL) {
		i++;
		if (i == buffer_mark - 1) {
			buffer_mark += BUFFER_SIZE;
			/* must protect against realloc failure memory leak */ 
			char **new_tokenized_input;
			if ((new_tokenized_input = realloc((*tokenized_input), sizeof(char*)*buffer_mark)) == NULL) {
				return EXIT;
			}
			(*tokenized_input) = new_tokenized_input;
		}
		(*tokenized_input)[i] = strtok(NULL, " ");
	}
	(*tokenized_input)[i] = NULL;
	return CONTINUE;
}

int is_a_deliminator(char *s) {
	for (int i = 0; i < NUM_DELIMINATORS; i++) {
		if (strncmp(s, command_delimintators[i], 1) == 0) {
			return TRUE;
		}	
	}
	return FALSE;
}

char *get_next_token()
{
	int count = 0;
	int is_null = FALSE;
	char *token;
	if (strncmp(t->pos, "\0", 1) == 0) {
		return NULL;
	}
	else if (is_a_deliminator(t->pos)) {
		token = t->pos;
		t->pos++;
		return token;
	}
	else {
		while (!(is_a_deliminator(t->pos))) {
			count++;
			t->pos++;
		}
		if (strncmp(t->pos, "\0", 1) == 0) { t->pos--, count--; }
		token = malloc(count + 1);
		t->pos -= count;
		for (int i = 0; i <= count; i++) {
			token[i] = *(t->pos);
			t->pos++;
		}
		token[count+1] = '\0'; 
		return token;
	}
}

int parse(job *current_job) 
{
	char *line;

	line = readline(PROMPT);

	/* handle c-d */
	if (line == NULL) {
		return EXIT;
	}

	/* handle return */
	if (strcmp(line,"") == 0) {
		return EXIT;
	}

	t = malloc(sizeof(tokenizer));
	
	t->str = line;
	t->pos = &((t->str)[0]);

	char *token;
	char **tokenized_process;
	int number_processes_in_job = 0;

	// process *cur_process = current_job->first_process;
		
	while((token = get_next_token()) != NULL) {
		if (token == NULL) {
			// cur_process = NULL;
		}
		else {
			// split_white_space(&token, &tokenized_process);
			// cur_process->args = tokenized_process;
			// process *next;
			// cur_process->next_process = next;
		}
		printf("%s \n", token);
	}

	return 1;
}

