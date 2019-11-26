#define COMMAND_LINE_SIZE 1024   // màxim número de línies que llegirem
#define ARGS_SIZE 64             // màxim número d'elements a l'array d'arguments
#define N_JOBS 4                 // màxim número de treballs en segon pla
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

int imprimir_prompt();
char *read_line(char *line);
int execute_line(char *line);

int parse_args(char **args, char *line);

int check_internal(char **args); 
int internal_cd(char **args); 
int internal_export(char **args); 
int internal_source(char **args); 
int internal_jobs(char **args);
int internal_fg(char **args);
int internal_bg(char **args);

int external_command(char **args);
void reaper(int signum);
void ctrlc(int signum);
void ctrlz(int signum);

int jobs_list_add(pid_t pid, char status, char *command_line);
int jobs_list_find(pid_t pid);
int jobs_list_remove(int pos);
int is_background(char **args);

int is_output_redirection(char **args);

struct info_process {
    pid_t pid;
    char status;
    char command_line[COMMAND_LINE_SIZE];
};

static char *g_argv; // Contindrà l'argument amb el que hem inicialitzat el programa
static int n_pids;   // Contindrà el número de treballs en segon pla que tenim al moment
static struct info_process jobs_list[N_JOBS + 1];