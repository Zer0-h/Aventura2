#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#define N_JOBS 64
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
/* Las llamadas al sistema están recopiladas en la sección 2 de Man. 
 * Ejemplos en nuestra aventura: open(), close(), chdir(), getcwd(), 
 * dup(), dup2(), execvp(), exit(), fork(), getpid(), kill(), pause(), 
 * signal(), wait(), waitpid(). Hay algunas llamadas al sistema que 
 * siempre tienen éxito, y no retornan ningún valor para indicar si 
 * se ha producido un error; ejemplos en nuestra aventura: getpid(), 
 * getppid()
 */

char *read_line(char *line); 
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args); 
int internal_cd(char **args); 
int internal_export(char **args); 
int internal_source(char **args); 
int internal_jobs(char **args); 
int imprimir_prompt();
int external_command(char **args);
void reaper(int signum);
void ctrlc(int signum);


struct info_process {
    pid_t pid;
    char status; // ’E’, ‘D’, ‘F’
    char command_line[COMMAND_LINE_SIZE]; // Comando
};

static char *g_argv;
static struct info_process jobs_list[N_JOBS];

int main(int argc, char *argv[]){
    char line[COMMAND_LINE_SIZE];
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    g_argv = argv[0];
    jobs_list[0].pid = 0;
    memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
    while (read_line(line)){
        execute_line(line);
    }
    return 0;
}

int imprimir_prompt(){
    char *user = getenv("USER"), *home = getenv("HOME"), pwd[COMMAND_LINE_SIZE];
    int len_home = strlen(home);
    if (*getcwd(pwd,COMMAND_LINE_SIZE) == -1)
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
    if (strncmp(pwd,home,len_home) == 0){
        char final_pwd[strlen(pwd) - len_home + 1];
        memset(final_pwd,0,sizeof(final_pwd)); // Ho possam tot a 0 perque no hi hagi caràcters que no volem al principi (donava problemes sense això)
        final_pwd[0] = '~';
        strcat(final_pwd,pwd+len_home);
        printf("\033[1;31m%s\033[0m:\033[1;32m%s\033[0m$ ",user,final_pwd);
    } else {
        printf("\033[1;31m%s\033[0m:\033[1;32m%s\033[0m$ ",user,pwd);
    }
    return 0;
}

char *read_line(char *line){
    char * ptr;
    imprimir_prompt();
    fflush(stdout);
    ptr = fgets (line, COMMAND_LINE_SIZE, stdin);
    if (!ptr) {  //ptr==0
        printf("\r");
       if (feof(stdin)) { //feof(stdin!=0)
           //printf("Has pulsado Ctrl+D");
           exit(0);
       } else {
           ptr = line; // si no al pulsar inicialmente CTRL+C sale fuera del shell
           ptr[0] = 0; // Si se omite esta línea aparece error ejecución ": no se encontró la orden"*/
       }
   }
    return ptr;
}

int execute_line(char *line){
    char *args[ARGS_SIZE];
    strcpy(jobs_list[0].command_line,line);
    jobs_list[0].command_line[strlen(line)-1] = 0; // Eliminam el caracter \n
    parse_args(args,line);
    if (args[0]){
        if (!check_internal(args)){
            external_command(args);
        }
    }
    return 0;
}

int parse_args(char **args, char *line){
    int token_counter = 0;
    char *token, delim[5] = " \t\r\n";
    token = strtok(line,delim);
    while (token){
        if (token[0] == '#'){break;}
        args[token_counter] = token;
        token = strtok(NULL, delim);
        token_counter++;
    }
    args[token_counter] = NULL;
    return token_counter;
}

int check_internal(char **args){
    if (strcmp(args[0],"cd") == 0){
        return internal_cd(args);
    } else if (strcmp(args[0],"export") == 0){
        return internal_export(args);
    } else if (strcmp(args[0],"source") == 0){
        return internal_source(args);
    } else if (strcmp(args[0],"jobs") == 0){
        return internal_jobs(args);
    } else if (strcmp(args[0],"exit") == 0){
        exit(0);
    }
    return 0;
}

/*
S'ha comprobat que funcioni per a directoris aparentment conflictius, un cas que funciona és:
cd Aventura2/'prueba dir'/p\r\u\e\b\a/\"/'pr\"ueba dir larga'/pr\'\"\\ueba/prueba\ dir
que ens duu al directori /home/user/Aventura2/prueba dir/prueba/"/pr"ueba dir larga/pr'"\ueba/prueba dir
*/

int internal_cd(char **args){
    char arg[COMMAND_LINE_SIZE];
    memset(arg,0,sizeof(arg));
    if (args[1] == NULL){
        strcpy(arg,getenv("HOME"));
    }else if(strpbrk(args[1],"\'\"\\")){
        if (args[1][0] == '/')
            arg[0] = '/';
        for (int i = 1; args[i]; i++){
            char* token; 
            char* rest = args[i];
            while (token = strtok_r(rest,"/",&rest)){
                char check1 = token[0];
                char *check2 = &token[strlen(token) -1];
                if (check1 == '\"' || check1 == '\'')
                    token++;
                if(*check2 == '\"' || *check2 == '\'' ){
                    if (*(check2 -1) == '\\')
                        *(check2-1) = *check2;
                    *check2 = 0;
                }
                for (token=strtok(token,"\\"); token; token =strtok(NULL,"\\")){
                    if (*(token -1) == '\\') /* Per solucionar el cas si introdueixes \\ */
                        token--;
                    strcat(arg,token);
                }
                strcat(arg,"/");
            }
            if (args[i+1])
                arg[strlen(arg) - 1] = ' ';
        }
     } else{
        strcpy(arg,args[1]);
    }
    if (chdir(arg) == -1){
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
    }
    return 1;
}

int internal_export(char **args){
    int error = 1;
    char *env[2];
    env[0] = strtok(args[1],"=");
    if (env[1] = strtok(NULL,"")){
        if (setenv(env[0],env[1],1) == -1)
            fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
    }
    else
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
    return 1;
}

int internal_source(char **args){
    if (args[1]){
        char line[COMMAND_LINE_SIZE];
        FILE *fp;
        if (fp = fopen(args[1],"r")){
            while (fgets(line,150, fp)){
                fflush(fp);
                execute_line(line);
            }
        fclose(fp);
        } else
            fprintf(stderr,"Error: No se ha encontrado el archivo '%s'\n",args[1]);
    } else
        fprintf(stderr,"Error de sintaxis. Uso: source <nombre_fichero>\n");
    return 1;
}

int internal_jobs(char **args){
    printf("[internal_jobs()→Esta función mostrará el PID de los procesos que no estén en foreground]\n");
    return 1;
}
/*
jobs son procesos que dependan del terminal (que esten en segundo plano y los que yo detenga 'ctrlz')
*/

int external_command(char **args){
    pid_t pid;
    pid = fork();
    if (pid == 0){
        signal(SIGCHLD,SIG_DFL);
        signal(SIGINT, SIG_IGN); // Ignoram senyal ctrl+c general
        printf("[execute_line()→ PID hijo: %d (%s)]\n",getpid(),jobs_list[0].command_line);
        execvp(args[0],args);
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
        exit(1);
    } else if (pid > 0){
        printf("[execute_line()→ PID padre: %d (%s)]\n",getpid(),g_argv);
        jobs_list[0].pid = pid;
        while (jobs_list[0].pid)
            pause();

    } else {
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
        exit(1);
    }
    return 0;
}

void reaper(int signum){
    pid_t pid;
    int status;
    signal(SIGCHLD,reaper);
    while ((pid=waitpid((pid_t)(-1), &status, WNOHANG)) > 0 ){
        if (WIFEXITED(status)){
            printf("[reaper()→ Proceso hijo %d (%s) finalizado con exit code %d]\n",pid,jobs_list[0].command_line,WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[reaper()→ Proceso hijo %d (%s) finalizado por señal %d]\n",pid,jobs_list[0].command_line,WTERMSIG(status));
        }
        if (pid == jobs_list[0].pid){
            jobs_list[0].pid = 0;
            memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
        }
    }
}

void ctrlc(int signum){
    signal(SIGINT, ctrlc);
    printf("\n");
    fflush(stdout);
    printf("[ctrlc()→ Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),g_argv,jobs_list[0].pid,jobs_list[0].command_line);
    if (jobs_list[0].pid > 0){
        if (strcmp(g_argv,jobs_list[0].command_line)){
            if (kill(jobs_list[0].pid,15) == 0){
                printf("[ctrlc()→ Señal 15 enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid,jobs_list[0].command_line,getpid(),g_argv);
            } else { // Cas es comando kill falla
                fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
            }
        } else {
            printf("[ctrlc()→ Señal 15 no enviada por %d (%s) debido a que su proceso en foreground es el shell]\n",getpid(),g_argv);
        }
    } else {
        printf("[ctrlc()→ Señal 15 no enviada por %d debido a que no hay proceso en foreground]\n",getpid());
    }
}
// NOT FINISHED, cuando tengo un shell dentro del shell y hago ctrl+d escribe el prompt y luego reaper lo sobrescribe (se puede hacer algo?)