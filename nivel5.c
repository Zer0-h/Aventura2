#include "nivel5.h"

int main(int argc, char *argv[]){
    char line[COMMAND_LINE_SIZE];
    signal(SIGCHLD, reaper);
    signal(SIGINT, ctrlc);
    signal(SIGTSTP,ctrlz);
    g_argv = argv[0];
    jobs_list[0].pid = 0;
    n_pids = 0;
    while (read_line(line)){
        execute_line(line);
    }
    return 0;
}

/*
 * int imprimir_prompt()
 * 
 * Amb la funció getenv("USER") i getcwd podem contruir el nostre prompt.
 * A més tal i com fa bash si el nostre directori comença per el directori
 * /home/user ho substituirem per ~
 * 
 */
int imprimir_prompt(){
    char *user = getenv("USER"), *home = getenv("HOME"), pwd[COMMAND_LINE_SIZE];
    int len_home = strlen(home);
    if (*getcwd(pwd,COMMAND_LINE_SIZE) == -1)
        perror("getcwd");
    if (strncmp(pwd,home,len_home) == 0){
        printf("\033[1;31m%s\033[0m:\033[1;32m~%s\033[0m$ ",user,pwd+len_home);
    } else {
        printf("\033[1;31m%s\033[0m:\033[1;32m%s\033[0m$ ",user,pwd);
    }
    return 0;
}

/*
 * char *read_line(char *line)
 * 
 * Crida a imprimir_prompt i llegeix una línia de la consola
 * Torna el punter a la línia que hem llegit
 * 
 */
char *read_line(char *line){
    char * ptr;
    imprimir_prompt();
    fflush(stdout);
    ptr = fgets (line, COMMAND_LINE_SIZE, stdin);
    if (!ptr) {
        printf("\r");
       if (feof(stdin)) { // Si s'ha produit un EOF (hem pulsat CTRL+D) sortirem del shell
           imprimir_prompt(); // tal i com tenim a bash ens monstra el prompt + exit quan sortim amb ctrl+d
           printf("exit\n"); 
           exit(0);
       } else {
           ptr = line;
           ptr[0] = 0;
       }
   }
    return ptr;
}

/*
 * int execute line(char *line)
 * 
 * Cridarà a parse args per obtenir la línia que l'usuari ha introdui fragmentada en tokens.
 * Passam els tokens a la función boolena check_internal() perquè ens determini si és un comand
 * intern o extern * 
*/
int execute_line(char *line){
    char *args[ARGS_SIZE];
    strcpy(jobs_list[0].command_line,line);
    jobs_list[0].command_line[strlen(line)-1] = 0; // Eliminam el caràcter \n
    parse_args(args,line);
    if (args[0]){
        if (check_internal(args)){
            external_command(args);
        }
    }
    return 0;
}


/*
 * int parse_args(char **args, char *line)
 * 
 * Trosseja la línia obtinguda mitjançant la funció strtok(). Ignorarà els comentaris
 * que comencin per # i afegirà com a darrer argument a l'string de caràcters NULL.
 * Així en altres funcions podrem saber quan acaba es loop de llegir tokens.
 *  
 */ 
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


/*
 * int check_internal(char **args)
 * És una funció booleana que troba si args[0] és un ordre interna, si ho és l'executarà i tornarà 0.
 */
int check_internal(char **args){
    if (strcmp(args[0],"cd") == 0){
        internal_cd(args);
    } else if (strcmp(args[0],"export") == 0){
        internal_export(args);
    } else if (strcmp(args[0],"source") == 0){
        internal_source(args);
    } else if (strcmp(args[0],"jobs") == 0){
        internal_jobs(args);
    } else if (strcmp(args[0],"fg") == 0){
        internal_fg(args);
    } else if (strcmp(args[0],"bg") == 0){
        internal_bg(args);
    } else if (strcmp(args[0],"exit") == 0){
        exit(0);
    } else {
        return 1;
    }
    return 0;
}

//     INTERNAL COMMANDS

/*
S'ha comprobat que funcioni per a directoris aparentment conflictius, un cas que funciona com a bash és:
cd Aventura2/'prueba dir'/p\r\u\e\b\a/\"/'pr\"ueba dir larga'/pr\'\"\\ueba/prueba\ dir
que ens duu al directori /home/user/Aventura2/prueba dir/prueba/"/pr"ueba dir larga/pr'"\ueba/prueba dir
*/

/*
 * int internal_cd(char **args)
 * Utilitza la cridad al sistema chdir() per canviar de directori. Si l'argument de cd té \ , " o '
 * haurem de tractar els arguments que ens han passat.
 * El caràcter \ s'eliminarà i deixarem el caràcter que vengui després d'ell (encara que sigui una altra \).
 * A més si el possam al final d'una parauala indicam que volem un espai.
 * Tant " com ' indicaran el principi i el final d'un directori que contengui un nombre ams espais 
 * 
 * Si cd no té arguments canviarem al directori HOME.
 *  
 * exemple: cd sistemes\ operatius, cd 'sistemes operatius' i cd "sistemes operatius" ens duran al mateix directori que se diu <prueba dir>
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
        perror("chdir");
    }
    return 0;
}


/*
 * internal_export(char **args)
 * Descompossa en tokens l'argument NOM=VALOR. Si la sintaxis és incorrecta li notificarem a l'usuari.
 * Mitjançant la funció setenv() assignarem un valor a una variable d'entorn
*/
int internal_export(char **args){
    int error = 1;
    char *env[2];
    env[0] = strtok(args[1],"=");
    if (env[1] = strtok(NULL,"")){
        if (setenv(env[0],env[1],1) == -1)
            perror("setenv");
    }
    else
        fprintf(stderr, "Error de sintaxis. Uso: export Nombre=Valor\n");
    return 0;
}

/*
 * internal_source(char **args)
 * 
 * Aquesta funció obrirà un arxiu i executarà les ordres que hi ha en aquest línia per línia.
 * 
 * 
*/

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
            fprintf(stderr,"Source: No se ha encontrado el archivo '%s'\n",args[1]);
    } else
        fprintf(stderr,"Error de sintaxis. Uso: source <nombre_fichero>\n");
    return 0;
}


/*
 * int internal_jobs(char **args)
 * Si no hi ha cap argument imprimirà per pantalla l'identificador del treball, el PID, la línia de comands i l'status de tots els treballs que tenim actualment.
 * Si afegim un argument només imprimirà el treball que li hem indicat mitjançant aquest.
*/
int internal_jobs(char **args){
    if (args[1]){
        int pos = atoi(args[1]);
        if (pos < 1 || pos > n_pids){
            fprintf(stderr,"jobs %s: no existe tal trabjo\n",args[1]);
        } else {
            printf("[%d] %d\t%c\t%s\n",pos,jobs_list[pos].pid,jobs_list[pos].status,jobs_list[pos].command_line);
        }
    } else {
        for (int i = 1; i <= n_pids; i++){
            printf("[%d] %d\t%c\t%s\n",i,jobs_list[i].pid,jobs_list[i].status,jobs_list[i].command_line);
        }
    }
    return 0;
}

/*
internal_fg(char **args)
    Mou un treball a foreground.

    Possa el treball identificat per args[1] (JOB_SPEC) a foreground.
    Si JOB_SPEC no està present o 
*/
int internal_fg(char **args){
    if (args[1]){
        int pos = atoi(args[1]);
        if (pos < 1 || pos > n_pids){
            fprintf(stderr,"fg %s: no existe tal trabjo\n",args[1]);
            return 0;
        }
        if (jobs_list[pos].status == 'D'){
            jobs_list[pos].status = 'E';
            if (kill(jobs_list[pos].pid,18) == 0){
                printf("[internal_fg()→ Señal 18 (SIGCONT) enviada a %d (%s)]\n",jobs_list[pos].pid,jobs_list[pos].command_line);
            } else {
                perror("kill");
            }
        }
        if (jobs_list[pos].command_line[strlen(jobs_list[pos].command_line) -1] == '&'){
            jobs_list[pos].command_line[strlen(jobs_list[pos].command_line) -2] = 0;
        }
        strcpy(jobs_list[0].command_line,jobs_list[pos].command_line);
        jobs_list[0].pid = jobs_list[pos].pid;
        puts(jobs_list[pos].command_line);
        jobs_list_remove(pos);
        while (jobs_list[0].pid)
            pause();
    } else{
        fprintf(stderr, "Error de sintaxis. Uso: fg <job_id>\n");
    }
    return 0;
} // Que feim en el cas 'fg 1 asjdbisabdisbixoa', donam syntax error o feim simplement fg 1 i ignoram lo que ve després d'aquest
  // INFO: a bash ignoram lo que ve després del número
  // Cas sleep 20 &sleep 20 a bash cream un procés a background i un a foreground, que feim aqui? modificar parse_args?

int internal_bg(char **args){
    if (args[1]){
        int pos = atoi(args[1]);
        if (pos < 1 || pos > n_pids){
            fprintf(stderr,"bg %s: no existe tal trabjo\n",args[1]);
            return 0;
        }
        if (jobs_list[pos].status == 'E'){
            fprintf(stderr,"bg: el trabajo %d ja está en segundo plano\n",pos);
            return 0;
        }
        jobs_list[pos].status = 'E';
        strcat(jobs_list[pos].command_line," &");
        if (kill(jobs_list[pos].pid,18) == 0){
            printf("[internal_bg()→ señal 18 (SIGCONT) enviada a %d (%s)]\n",jobs_list[pos].pid,jobs_list[pos].command_line);
            printf("[%d] %d\t%c\t%s\n",pos,jobs_list[pos].pid,jobs_list[pos].status,jobs_list[pos].command_line);
        } else {
            perror("kill");
        }
    } else {
        fprintf(stderr, "Error de sintaxis. Uso: bg <job_id>\n");
    }
    return 0;
}

int external_command(char **args){
    pid_t pid;
    int bkg = is_background(args);
    if (bkg && n_pids >= N_JOBS -1){
        fprintf(stderr,"Error, no se pueden añadir más procesos en segundo plano\n");
        return 1;
    }
    pid = fork();
    if (pid == 0){
        signal(SIGCHLD,SIG_DFL);
        signal(SIGINT, SIG_IGN); // Ignoram senyal ctrl+c general
        if (bkg){
            signal(SIGTSTP,SIG_IGN);
        }
        execvp(args[0],args);
        fprintf(stderr,"%s: no se ha encontrado el comando\n",args[0]);
        exit(1);
    } else if (pid > 0){
        printf("[execute_line()→ PID padre: %d (%s)]\n",getpid(),g_argv);
        printf("[execute_line()→ PID hijo: %d (%s)]\n",pid,jobs_list[0].command_line);
        if (bkg){
            jobs_list_add(pid,'E',jobs_list[0].command_line);
            memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
        } else{
            jobs_list[0].pid = pid;
            while (jobs_list[0].pid)
                pause();
        }
    } else {
        perror("fork");
        exit(1);
    }
    return 0;
}

int is_background(char **args){
    for (int i = 1; args[i] ; i++){
        if (args[i][0] == '&'){
            args[i] = NULL;
            return 1; // True
        }
    }
    return 0; // False
}

int jobs_list_add(pid_t pid, char status, char *command_line){
    n_pids++;
    jobs_list[n_pids].pid = pid;
    jobs_list[n_pids].status = status;
    strcpy(jobs_list[n_pids].command_line,command_line);
    printf("[%d] %d\t%c\t%s\n",n_pids,jobs_list[n_pids].pid,jobs_list[n_pids].status,jobs_list[n_pids].command_line);
    return 0;
} // Aquí no tenim condicionals perquè en casos on la llista de jobs estigui plena ens interesa saber-ho abans de cridar al comand jobs_list_add i
  // mai el cridarem si ja esteim en es límit de jobs. Així si volem fer un procés en background i ja esteim al límit ni tal sols cridarem en es fork.
  // Si pulsam ctrl+z i esteim plens, no l'aturarem.

int jobs_list_find(pid_t pid){
    int position = 1;
    while (position < N_JOBS){
        if (jobs_list[position].pid ==  pid)
            return position;
        position++;
    }
    return 0; // En el cas que no el trobem, se podria llevar, ja qe amb les actualitzacions que hem fet del codi mai arribarem al cas on demanam trobar
              // un procés que no estigui a la llista.
}

int  jobs_list_remove(int pos){
    jobs_list[pos] = jobs_list[n_pids];
    jobs_list[n_pids].pid = 0;
    n_pids--;
    return 0;
}

/*******************************
        TO DO LIST
********************************

Recolocar funcions perquè el seu ordre tengui sentit
Revisar si totes les cridades al sistema tenen un control d'errors (al acabar tot)
    Aquestes son open(), close(), chdir(), getcwd(), dup(), dup2(), execvp(), exit(), fork(), getpid(), kill(), pause(), 
    signal(), wait(), waitpid(). Las llamadas al sistema están recopiladas en la sección 2 de Man
Revisar internal_cd i provar més casos.
README.txt
*/

//      CONTROLADORS

void reaper(int signum){
    pid_t pid;
    int status;
    signal(SIGCHLD,reaper);
    while ((pid=waitpid((pid_t)(-1), &status, WNOHANG)) > 0 ){
        int position;
        if (pid == jobs_list[0].pid){
            if (WIFEXITED(status)){
                printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado con exit code %d]\n",pid,jobs_list[0].command_line,WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado por señal %d]\n",pid,jobs_list[0].command_line,WTERMSIG(status));
            }
            jobs_list[0].pid = 0;
            memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
        } else if (position = jobs_list_find(pid)){ // Si trobam es pid a jobs_list
            printf("\n");
            if (WIFEXITED(status)){
                printf("[reaper()→ Proceso hijo %d en background (%s) finalizado con exit code %d]\n",pid,jobs_list[position].command_line,WEXITSTATUS(status));
                fprintf(stderr,"Terminado PID %d (%s) en jobs_list[%d] con status %d\n",pid,jobs_list[position].command_line,position,WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[reaper()→ Proceso hijo %d en background (%s) finalizado por señal %d]\n",pid,jobs_list[position].command_line,WTERMSIG(status));
                fprintf(stderr,"Terminado PID %d (%s) en jobs_list[%d] por señal %d\n",pid,jobs_list[position].command_line,position,WTERMSIG(status));
            }
            jobs_list_remove(position);
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
            } else {
                perror("kill");
            }
        } else {
            printf("[ctrlc()→ Señal 15 no enviada por %d (%s) debido a que su proceso en foreground es el shell]\n",getpid(),g_argv);
        }
    } else {
        printf("[ctrlc()→ Señal 15 no enviada por %d debido a que no hay proceso en foreground]\n",getpid());
    }
}

void ctrlz(int signum){
    signal(SIGTSTP,ctrlz);
    printf("\n[ctrlz()→ Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),g_argv,jobs_list[0].pid,jobs_list[0].command_line);
    if (jobs_list[0].pid > 0){
        if (strcmp(g_argv,jobs_list[0].command_line)){
            if (n_pids >= N_JOBS -1){
                fprintf(stderr,"No se pueden añadir más procesos en segundo plano, el proceso seguirá en foreground\n");
                if(kill(jobs_list[0].pid,18) == -1) // Per alguna raó aturava el procés encara que no hi arribés a enviar SIGTSTP, aquesta passa és obligatoria
                    perror("kill");
            } else if(kill(jobs_list[0].pid,20) == 0){
                printf("[ctrlz()→ Señal 20 (SIGTSTP) enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid,jobs_list[0].command_line,getpid(),g_argv);
                jobs_list_add(jobs_list[0].pid,'D',jobs_list[0].command_line);
                jobs_list[0].pid = 0;
                memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
            } else {
                perror("kill");
            }
        } else {
            printf("[ctrlz()→ Señal 20 no enviada por %d (%s) debido a que su proceso en foreground es el shell]\n",getpid(),g_argv);
        }
    } else {
        printf("[ctrlz()→ Señal 20 no enviada por %d debido a que no hay proceso en foreground]\n",getpid());
    }
}