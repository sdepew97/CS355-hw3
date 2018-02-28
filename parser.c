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
char *PROMPT = "$ ";

tokenizer *t = NULL;
tokenizer *pt;
extern job *all_jobs;
extern background_job *all_background_jobs;

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
	char *token = NULL;
	if (strncmp(t->pos, "\0", 1) == 0) {
		return NULL;
	}
	else if (is_a_deliminator(t->pos)) {
		/* might want to make this where syntax errors occur */
		fprintf(stderr, "Syntax error near unexpected token %s \n", t->pos);
		exit(EXIT_FAILURE);
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


int lengthOf(char *str){
    int i = 0;
    while (str[i] != '\0') {
        i++;
    }
    return i;
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
//    line = readline(PROMPT);
	// line = "python test.py &  python test.py & python test.py & python test.py & python test.py & python test.py & python test.py & python test.py & python test.py &";
	line = malloc(5);
	line = "test\n";

	/* handle c-d */
	if (line == NULL) {
		return EXIT;
	}

	/* handle return */
	if (strcmp(line,"") == 0) {
		if(line != NULL) {
			free(line);
		}
		return EXIT;
	}

	t = malloc(sizeof(tokenizer));
	t->str = line;
	t->pos = &((t->str)[0]);

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
	all_jobs = cur_job;

	while ((token = get_next_token()) != NULL) {
		job *new_job;
		new_job = malloc(sizeof(job));
		new_job->next_job = NULL;

		char *fjs = malloc(lengthOf(token) + 1);

		cur_job->full_job_string = strcpy(fjs, token);
		cur_job->job_string = token;
		cur_job->next_job = new_job;
		cur_job->pgid = 0;
		cur_job->stdin = STDIN_FILENO;
		cur_job->stdout = STDOUT_FILENO;
		cur_job->stderr = STDERR_FILENO;

		if (strncmp(last_element_of(cur_job->job_string), "&", 1) == 0) {
			int l = strlen(cur_job->job_string);
			(cur_job->job_string)[l-1] = '\0';
			cur_job->run_in_background = TRUE;
		}
        else {
            cur_job->run_in_background = FALSE;
        }
		if (strncmp(last_element_of(cur_job->job_string), ";", 1) == 0) {
			int l = strlen(cur_job->job_string);
			(cur_job->job_string)[l-1] = '\0';
		}

		cur_job = new_job;
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
		cur_process->pid = 0;
		cur_process->status = 0;

		temp_job->first_process = cur_process;

		/* clean up jobs because it leaves one  */
		if (temp_job->next_job->next_job == NULL) { 
			temp_job->next_job = NULL; 
		}

		temp_job = temp_job->next_job;

	}

	free(t);
	free(line); 
	free(cur_job);
	return num_jobs;
}


/* free memory associated with all jobs global */
void free_all_jobs() {
    while (all_jobs != NULL) {
        free(all_jobs->job_string);
        free(all_jobs->full_job_string);
        process *temp_p = all_jobs->first_process;
        temp_p->next_process = NULL;
        while (temp_p != NULL) {
			if(temp_p->args!=NULL) {
				free(temp_p->args);
			}
            process *t = temp_p->next_process;
            free(temp_p);
            temp_p = t;
        }
        job *j = all_jobs->next_job;
        free(all_jobs);
        all_jobs = j;
    } 
}

void free_background_jobs() {
    while (all_background_jobs != NULL) {
        free(all_background_jobs->job_string);
        all_background_jobs = all_background_jobs->next_background_job;
    }
}

void free_background_job(background_job *job1) {
    if(job1->job_string != NULL) {
        free(job1->job_string);
    }
    if(job1 != NULL) {
        free(job1);
    }
}
