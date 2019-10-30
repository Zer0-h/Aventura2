#define COMMAND_LINE_SIZE 1024
#define ARGS_SIZE 64
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

char *read_line(char *line); 
int execute_line(char *line);
int parse_args(char **args, char *line);
int check_internal(char **args); 
int internal_cd(char **args); 
int internal_export(char **args); 
int internal_source(char **args); 
int internal_jobs(char **args); 
int imprimir_prompt();

int main(){
    char line[COMMAND_LINE_SIZE];
    while (read_line(line)){
        execute_line(line);
    }
    return 0;
}

int imprimir_prompt(){
    char *user = getenv("USER"), *home = getenv("HOME"), pwd[COMMAND_LINE_SIZE];
    getcwd(pwd,COMMAND_LINE_SIZE);
    int len_home = strlen(home);
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
    imprimir_prompt();
    fgets (line, COMMAND_LINE_SIZE, stdin);
    return line;
}

int execute_line(char *line){
    char *args[ARGS_SIZE];
    parse_args(args,line);
    check_internal(args);
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
    if (args[0]){ // Comprovam que el primer no es nul, strcmp donaria un error si fos així
        if (strcmp(args[0],"cd") == 0){
           internal_cd(args);
        } else if (strcmp(args[0],"export") == 0){
           internal_export(args);
        } else if (strcmp(args[0],"source") == 0){
           internal_source(args);
        } else if (strcmp(args[0],"jobs") == 0){
           internal_jobs(args);
        } else if (strcmp(args[0],"exit") == 0){
           exit(0);
        }
        return 1;
    }
    return 0;
}

int internal_cd(char **args){
    int cd;
    char arg[COMMAND_LINE_SIZE];
    memset(arg,0,sizeof(arg));
    if (args[1] == NULL){
        strcpy(arg,getenv("HOME"));
    }else if(strchr(args[1],'\"') || strchr(args[1],'\'') || strchr(args[1],'\\')){
        strcat(arg,args[1]);
        for (int i = 2; args[i]; i++){
            strcat(arg," ");
            strcat(arg,args[i]);
        }
        char *token, delim[4] = "\\,\'\"";
        token = strtok(arg,delim);
        memset(arg,0,sizeof(arg));
        while (token){
            strcat(arg,token);
            token = strtok(NULL,delim);
        }
    }else{
        strcpy(arg,args[1]);
    }
    cd = chdir(arg);
    if (cd == -1){
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
    }
    return 1;
}

int internal_export(char **args){
    printf("[internal_export()→Esta función asignará valores a variables de entorno]\n");
    return 1;
}

int internal_source(char **args){
    printf("[internal_source()→Esta función ejecutará un fichero de líneas de comandos]\n");
    return 1;
}

int internal_jobs(char **args){
    printf("[internal_jobs()→Esta función mostrará el PID de los procesos que no estén en foreground]\n");
    return 1;
}