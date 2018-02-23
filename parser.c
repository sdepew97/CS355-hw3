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

extern tokenizer *t;
extern tokenizer *pt;
extern job *all_jobs;

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
		/* might want to make this where syntax errors occur */
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
		token = malloc(sizeof(char*) * (count + 1));
		t->pos -= count;
		for (int i = 0; i <= count; i++) {
			token[i] = *(t->pos);
			t->pos++;
		}
		token[count+1] = '\0'; 
		return token;
	}
}

char last_element_of(char *str)
{
	int i = 0;
	while (str[i] != '\0') {
		i++;
	}
	return str[--i];
}

/*
 *
 * BUGS: Doesn't like things like &; or asddas &; which is fine but need to catch those and throw syntax error
 * 		- Currently just puts one process in one job, will expand once we get to piping b/ I don't want to do that right now
 */
int parse() 
{
	char *line;

	/* readline causes leak */
	line = "ls ;";
	// line = readline(PROMPT);

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

	// free(line);

	char *token;
	char **tokenized_process;
	int num_jobs = 0;

	while((token = get_next_token()) != NULL) {
		num_jobs++;
		free(token);
	}

	free(t);

	t = malloc(sizeof(tokenizer));
	t->str = line;
	t->pos = &((t->str)[0]);

	job *cur_job;
	cur_job = malloc(sizeof(job));

	all_jobs = cur_job;
	all_jobs->job_string = get_next_token();

	while ((token = get_next_token()) != NULL) {
		if (token == NULL) {
			cur_job->next_job = NULL;
		}
		else {
			// printf("%s\n", token);
			job *new_job;
			new_job = malloc(sizeof(job));
			new_job->job_string = token;

			if (last_element_of(new_job->job_string)) {
				new_job->run_in_background = TRUE;
			}

			cur_job->next_job = new_job;
			cur_job = new_job;
		}
	}

	/* need to add second layer of delimination for multiple processes within same job
	 * this should be done if we want to get piping and redirection working */
	job *temp_job = all_jobs; 
	while ((temp_job) != NULL) {
		process *cur_process;
		char **tokenized_process;
		cur_process = malloc(sizeof(process));

		split_white_space(&(temp_job->job_string), &(tokenized_process));

		cur_process->args = tokenized_process;
		temp_job->first_process = cur_process;

		temp_job = temp_job->next_job;
	}

	
	free(t);
	return num_jobs;

}

