#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "shell.h"
#include "parser.h"

#define BUFFER_SIZE 4096
#define CONTINUE 1
#define TRUE 1
#define FALSE 0
#define NUM_DELIMINATORS 3

char *command_delimintators[NUM_DELIMINATORS] = {"&" , ";", "\0"};
char *white_space_deliminator = " ";
char *PROMPT = ">> ";

tokenizer *t = NULL;
tokenizer *pt;
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

char *get_next_token() {
    int count = 0;
    int is_null = FALSE;
    char *token = NULL;
    if (strncmp(t->pos, "\0", 1) == 0) {
        return NULL;
    } else if (is_a_deliminator(t->pos)) {
        /* might want to make this where syntax errors occur */
        token = t->pos;
        t->pos++;
        return token;
    } else {
        while (!(is_a_deliminator(t->pos))) {
            count++;
            t->pos++;
        }
        if (strncmp(t->pos, "\0", 1) == 0) { t->pos--, count--; }
        token = malloc(sizeof(char *) * (count + 1));
        t->pos -= count;
        for (int i = 0; i <= count; i++) {
            token[i] = *(t->pos);
            t->pos++;
        }
        token[count + 1] = '\0';
        return token;
    }
}

char *last_element_of(char *str)
{
	char *i = &str[0];
	int j = 0;
	while (strncmp(i, "\0", 1) != 0) {
		i++;
		j++;
	}

	if (j == 0) { return "\0"; }
	else { return --i; }
}

/*
 * Transforms all jobs global into first job in job LL and returns the number of jobs to run
 * 
 * BUGS: Doesn't like things like &; or asddas &; which is fine but need to catch those and throw syntax error
 * 		>>> Currently just puts one process in one job, will expand once we get to piping b/ I don't want to deal right now
 */
int perform_parse() 
{
	char *line = NULL;

	/* readline causes leak */
	line = "exit\0";
//	line = readline(PROMPT);

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

//	free(line);

	char *token = NULL;
	char **tokenized_process = NULL;
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
	cur_job->job_string = get_next_token(); 

	cur_job->next_job = NULL;

	all_jobs = cur_job;

	if (strncmp(last_element_of(all_jobs->job_string), "&", 1) == 0) {
		all_jobs->run_in_background = TRUE;
	}

	while ((token = get_next_token()) != NULL) {
		if (token == NULL) {
			cur_job->next_job = NULL;
		}
		else {
			job *new_job;
			new_job = malloc(sizeof(job));
			new_job->job_string = token;

			if (strncmp(last_element_of(new_job->job_string), "&", 1) == 0) {
				new_job->run_in_background = TRUE;
			}

			cur_job->next_job = new_job;
			cur_job = new_job;
            //TODO: determine if this line is needed here free(token);
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
        cur_process->next_process = NULL;
		temp_job->first_process = cur_process;

		temp_job = temp_job->next_job;
	}
	
	free(t);
	return num_jobs;
}


/* free memory associated with all jobs global */
void free_all_jobs() {
    while (all_jobs != NULL) {
        free(all_jobs->job_string);
        process *temp_p = all_jobs->first_process;
        temp_p->next_process = NULL;
        while (temp_p != NULL) {
            free(temp_p->args);
            process *t = temp_p->next_process;
            free(temp_p);
            temp_p = t;
        }
        job *j = all_jobs->next_job;
        free(all_jobs);
        all_jobs = j;
    } 
}
