#include "nivel6.h"

/*
 * En aquest codi hem considerat que quan tornam 0 és success i 1 és failure.
 */

/*
 * int main
 * --------
 * Controlarem les senyals SIGCHLD, SIGINT i SIGTSTP amb les seves funcions
 * corresponents. Aquestes senyals venen donades per quan un fill acaba, quan
 * pulsam CTRL+C i quan pulsam CTRL+Z respectivament.
 * 
 * Inicialitzam les variables globals g_argv, n_pids i la estructura jobs_list.
 * g_argv contendrà el nom del programa que inicialitza es shell, n_pids el
 * número total de treballs en background i la estructura jobs_list contendrà
 * informació de cada treball (pid, estatus i la seva línia de comando),
 * reservarem la posició [0] per guardar dades del procés en foreground.
 * 
 * Entram en un bucle del que només podrem sortir amb el comando exit o CTRL+D
 * quan ens ensenya es prompt.
 */

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
 * int imprimir_prompt
 * -------------------
 * Amb la funció getenv("USER") i getcwd podem conseguir l'usuari i el directori
 * al que esteim, amb això podem contruir i imprimir el nostre prompt.
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
 * char *read_line
 * --------------- 
 * Crida a imprimir_prompt i llegeix una línia de la consola amb fgets(), si 
 * pulsam CTRL+D al principi d'una línia significa el final de la entrada stdin,
 * per tant hem de controlar que si el resultat de fgets() és NULL i s'ha
 * produit un EOF sortirem del shell amb exit.
 * 
 * Torna el punter a la línia que hem llegit
 */
char *read_line(char *line){
    char * ptr;
    imprimir_prompt();
    fflush(stdout);
    ptr = fgets (line, COMMAND_LINE_SIZE, stdin);
    if (!ptr) {
        printf("\r");
       if (feof(stdin)) {
           imprimir_prompt();
           printf("exit\n"); // mostrarem exit abans de sortir.
           exit(0);
       } else {
           ptr = line;
           ptr[0] = 0;
       }
   }
    return ptr;
}

/*
 * int execute line
 * ----------------
 * Cridarà a parse args per obtenir la línia que l'usuari ha introduit
 * fragmentada en tokens.
 * Passam els tokens a la función boolena check_internal() perquè ens determini 
 * si és un comando intern o extern i l'executarà de forma adecuada.
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
 * int parse_args
 * --------------
 * Trosseja la línia obtinguda mitjançant la funció strtok(). Ignorarà els 
 * comentaris (els que comencen per #) i afegirà com a darrer argument a 
 * l'string de caràcters NULL. Així en altres funcions podrem saber quan acaba 
 * es bucle de llegir tokens.
 * 
 * Exemples:
 *      pwd              -> ["pwd",NULL]
 *      pwd #comentari   -> ["pwd",NULL]
 *      pwd no#comentari -> ["pwd","no#comentari", NULL]
 * 
 * Torna el número de tokens sense contar es darrer (NULL)
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
 * int check_internal
 * ------------------
 * És una funció booleana que troba si args[0] és un comando intern amb strcmp()
 * si ho és l'executarà.
 * 
 * Tornarà 0 si és un comando intern i 1 en casd contrari.
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
 * int internal_cd
 * ---------------
 * Utilitza la cridad al sistema chdir() per canviar de directori.
 * 
 * Si l'argument té \, " o ' haurem de tractar els arguments que ens han passat.
 * 
 * El caràcter \ s'eliminarà i deixarem el caràcter que vengui després d'ell 
 * (encara que sigui un altre \). A més si el possam al final d'una parauala
 * indicam que volem un espai.
 * 
 * Tant " com ' indicaran el principi i el final d'un directori que contengui un
 * nombre ams espais.
 * 
 * Si cd no té arguments canviarem al directori HOME.
 * 
 * Exemples per accedir a un directori anomenat "sistemes operatius":
 *      cd sistemes\ operatius
 *      cd 'sistemes operatius'
 *      cd "sistemes operatius"
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
 * internal_export
 * ---------------
 * Descompossa en tokens l'argument NOM=VALOR. Si la sintaxis és incorrecta li
 * notificarem a l'usuari.
 * 
 * Mitjançant la funció setenv() assignarem un valor (VALOR) a una variable 
 * d'entorn (NOM).
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
 * internal_source
 * --------------- 
 * Aquesta funció obrirà un arxiu amb fopen(), llegirà línia a línea l'arxiu
 * mitjançant fgets() i la passarem a la funció execute_line(). Al acabar de
 * llegir l'arxiu el tancam amb fclose().
 * 
 * Si la sintaxis és incorrecta li notificarem a l'usuari.
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
            fprintf(stderr,"Source: No se ha encontrado el archivo '%s'\n",
                    args[1]);
    } else
        fprintf(stderr,"Error de sintaxis. Uso: source <nombre_fichero>\n");
    return 0;
}


/*
 * internal_jobs
 * -------------
 * Si no hi ha cap argument imprimirà per pantalla l'identificador del treball,
 * el PID, la línia de comands i l'status de tots els treballs que tenim.
 * 
 * Si afegim un o més arguments imprimirà els treballs que li hem indicat.
 */

int internal_jobs(char **args){
    if (args[1]){
        for (int i = 1; args[i] ; i++){
            int pos = atoi(args[i]);
            if (pos < 1 || pos > n_pids){
                fprintf(stderr,"jobs %s: no existe tal trabjo\n",args[i]);
            } else {
                printf("[%d] %d\t%c\t%s\n",pos,jobs_list[pos].pid,
                        jobs_list[pos].status,jobs_list[pos].command_line);
            }
        }
    } else {
        for (int i = 1; i <= n_pids; i++){
            printf("[%d] %d\t%c\t%s\n",i,jobs_list[i].pid,jobs_list[i].status,
                    jobs_list[i].command_line);
        }
    }
    return 0;
}

/*
 * int internal_fg
 * ---------------
 * Possa el treball que li hem indicat amb <fg jobID> a foreground, si estaba
 * detingut, el resumim.
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
                printf("[internal_fg()→ Señal 18 (SIGCONT) enviada a %d (%s)]\n"
                        ,jobs_list[pos].pid,jobs_list[pos].command_line);
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
} // IMPORTANT implementar que pugui agafar més d'un jobID
  // per exemple fer fg jobID1 jobID2 jobID3 POSSARHO A README.TXT AL ACABAR!"!!!!!!!!!"





  // Cas sleep 20 &sleep 20 a bash cream un procés a background i un a foreground, que feim aqui? modificar parse_args?

/*
 * int internal_bg
 * ---------------
 * Si el treball que li hem indicat està detingut el resumim i el deixam a
 * background.
 */

int internal_bg(char **args){
    if (args[1]){
        for (int i = 1; args[i] ; i++){
            int pos = atoi(args[i]);
            if (pos < 1 || pos > n_pids){
                fprintf(stderr,"bg %s: no existe tal trabjo\n",args[i]);
            } else if (jobs_list[pos].status == 'E'){
                fprintf(stderr,"bg: el trabajo %d ja está en segundo plano\n",pos);
            } else {
                jobs_list[pos].status = 'E';
                strcat(jobs_list[pos].command_line," &");
                if (kill(jobs_list[pos].pid,18) == 0){
                    printf("[internal_bg()→ señal 18 (SIGCONT) enviada a %d (%s)]\n",jobs_list[pos].pid,jobs_list[pos].command_line);
                    printf("[%d] %d\t%c\t%s\n",pos,jobs_list[pos].pid,jobs_list[pos].status,jobs_list[pos].command_line);
                } else {
                    perror("kill");
                }
            }
        }
    } else {
        fprintf(stderr, "Error de sintaxis. Uso: bg <job_id>\n");
    }
    return 0;
}

/*
 * int external_command
 * --------------------
 * Comprobarà si el nostre comando és background i farà el control d'errors
 * corresponent. Cridarem a fork per crear un fill.
 * 
 * El procés fill:
 *      Ignorarà la interrupció CTRL+C, executarà l'acció per defecte per a la
 *      senyal SIGCHLD i ignorarà la senyal CTRL+Z si és un proces en background
 *      i executarà l'acció per defecte si és en foreground. El pare sirà
 *      l'encarregat de tractar totes aquestes senyals.
 * 
 *      Cridarà a la funció execvp() per executar el comando extern solicitat.
 * 
 * El procés pare:
 *      Si el comando és background l'afegirem a la llista de treballs amb la
 *      funció jobs_list_add().
 * 
 *      Si no, donarà els valors del procés en foreground i executarà un pause()
 *      mentres hi hagui un procés executant-se en foreground.
 */

int external_command(char **args){
    pid_t pid;
    int bkg = is_background(args);
    if (bkg == 0 && n_pids >= N_JOBS){
        fprintf(stderr,"Error, no se pueden añadir más procesos en segundo plano\n");
        return 1;
    }
    pid = fork();
    if (pid == 0){
        signal(SIGCHLD,SIG_DFL);
        signal(SIGINT, SIG_IGN);
        if (bkg == 0){
            signal(SIGTSTP,SIG_IGN);
        } else{
            signal(SIGTSTP,SIG_DFL); // Arreglar más tarde!!!!!
        }
        is_output_redirection(args);
        execvp(args[0],args);signal(SIGTSTP,SIG_IGN);
        fprintf(stderr,"%s: no se ha encontrado el comando\n",args[0]);
        exit(1);
    } else if (pid > 0){
        printf("[execute_line()→ PID padre: %d (%s)]\n",getpid(),g_argv);
        printf("[execute_line()→ PID hijo: %d (%s)]\n",pid,jobs_list[0].command_line);
        if (bkg == 0){
            jobs_list_add(pid,'E',jobs_list[0].command_line);
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

/*
 * int is_background
 * -----------------
 * Analitza a la línea de comandos si hi ha un & al final.
 * 
 * Torna 0 si és background i 1 si no ho és.
 */

int is_background(char **args){
    int length = 1;
    while (args[length]){
        length++;
    }
    if(length > 1 && strcmp(args[length -1],"&") == 0){
        args[length -1] = NULL; // Amb length > 1 arreglam el cas on introduim només una &
        return 0; // True
    } else{
        return 1; // False
    }
}

/*
 * int is_output_redirection
 * -------------------------
 * Funció que recorr la llista d'arguments cercant '>' seguit d'un únic token
 * que sigui el nom de l'arxiu.
 * 
 * Modificarem args per a que ho accepti execvp() i redireccionarem la sortida
 * a l'arxiu que hem indicat.
 */

int is_output_redirection(char **args){
    int length = 1;
    while (args[length]){
        length++;
    }

    // En lloc de cercar '>' per tot args ho cercarem a la penúltima posició.

    if(length > 2 && strcmp(args[length -2],">") == 0){
        int fd;
        fd = open(args[length-1], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        if (dup2(fd,1) == -1){
            perror("dup2");
        }
        close(fd);
        args[length -2] = NULL;
    }
    return 0;
}

/*
 * int jobs_list_add
 * -----------------
 * Afegim un nou element a l'array de jobs_list indicada per la variable global
 * n_pids
 */

int jobs_list_add(pid_t pid, char status, char *command_line){
    n_pids++;
    jobs_list[n_pids].pid = pid;
    jobs_list[n_pids].status = status;
    strcpy(jobs_list[n_pids].command_line,command_line);
    printf("[%d] %d\t%c\t%s\n",n_pids,jobs_list[n_pids].pid,jobs_list[n_pids].status,jobs_list[n_pids].command_line);
    jobs_list[0].pid = 0;
    memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
    return 0;
} // Aquí no tenim condicionals perquè en casos on la llista de jobs estigui plena ens interesa saber-ho abans de cridar al comand jobs_list_add i
  // mai el cridarem si ja esteim en es límit de jobs. Així si volem fer un procés en background i ja esteim al límit ni tal sols cridarem en es fork.
  // Si pulsam ctrl+z i esteim plens, no l'aturarem.

/*
 * int jobs_list_find
 * ------------------
 * Cerca a l'array del treball el PID que agafa com argument.
 * 
 * Torna la posició en que es troba aquest PID, si no l'ha trobat tornarà 0.
 */

int jobs_list_find(pid_t pid){
    int position = 1;
    while (position < N_JOBS +1){
        if (jobs_list[position].pid ==  pid)
            return position;
        position++;
    }
    return 0; // En el cas que no el trobem, se podria llevar, ja qe amb les actualitzacions que hem fet del codi mai arribarem al cas on demanam trobar
              // un procés que no estigui a la llista.
}

/*
 * int jobs_list_remove
 * --------------------
 * Agafa com argument la posició de l'array del treball que hem d'eliminat i mou
 * el registre del darrer procés de la llista a la posició que hem eliminat.
 * Reduim el valor de n_pids per 1.
 */

int jobs_list_remove(int pos){
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
Acabar README.txt

Implementar flags per a funcions internes.

Possible solució fer que el fill sempre ignori SIGTSTP, demanar perque funciona
Canviar numeros per senyals, pareix que estan incorrectes?

Canviar strcpy a execute line a jobs_list[0].command_line i possar-ho dins parse_args
*/

//      CONTROLADORS


/*
 * void reaper
 * -----------
 * Controlador per a la senyal SIGCHLD (senyal enviada a un procés quan un dels
 * seus processos fills termina).
 * 
 * Amb waipid() podem saber quin fill és el que termina. Tenim dos casos:
 * 
 * El fill estava en foreground:
 *      Reseteam la posició 0 de l'array jobs_list (que conté informació sobre 
 *      el procés en foreground)
 * 
 * El fill estava en background:
 *      Trobam la seva posició amb jobs_list_find, imprimim que un procés en
 *      background ha terminat (amb les seves dades corresponents) i cridam a
 *      la funció jobs_list_remove per eliminar el procés de la llista.
 */

void reaper(int signum){
    pid_t pid;
    int status;
    signal(SIGCHLD,reaper); // Tornam a assignar la senyal perque en alguns sistemes després de rebrer-la per primera vegada ña restaura a l'acció per defecte
    while ((pid=waitpid((pid_t)(-1), &status, WNOHANG)) > 0 ){
        if (pid == jobs_list[0].pid){
            if (WIFEXITED(status)){
                printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado con exit code %d]\n",pid,jobs_list[0].command_line,WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[reaper()→ Proceso hijo %d en foreground (%s) finalizado por señal %d]\n",pid,jobs_list[0].command_line,WTERMSIG(status));
            }
            jobs_list[0].pid = 0;
            memset(jobs_list[0].command_line,0,sizeof(jobs_list[0].command_line));
        } else{
            int position = jobs_list_find(pid);
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

/*
 * void ctrlc
 * ----------
 * Controlador per a la senyal SIGINT (senyal enviada amb CTRL+C).
 * Com que els processos fills ignoren la senyal SIGINT el que farem nosaltres
 * és enviar la senyal SIGTERM a través del procés pare.
 * 
 * Només enviarem la senyal SIGTERM si el procés està a foreground i a més no
 * és el nostre propi minishell
 */

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

/*
 * void ctrlz
 * ----------
 * Controlador per a la senyal SIGTSTP (senyal enviada amb CTRL+Z).
 * 
 * Només enviarem la senyal SIGTSTP si el procés està a foreground i a més no
 * és el nostre propi minishell.
 */

void ctrlz(int signum){
    signal(SIGTSTP,ctrlz);
    printf("\n[ctrlz()→ Soy el proceso con PID %d (%s), el proceso en foreground es %d (%s)]\n",getpid(),g_argv,jobs_list[0].pid,jobs_list[0].command_line);
    if (jobs_list[0].pid > 0){
        if (strcmp(g_argv,jobs_list[0].command_line)){
            if (n_pids >= N_JOBS){
                fprintf(stderr,"No se pueden añadir más procesos en segundo plano, el proceso seguirá en foreground\n");
                if(kill(jobs_list[0].pid,18) == -1) // arreglar més tard!!!!!!!!!!!!!!!!!!!
                    perror("kill");
            } else if(kill(jobs_list[0].pid,20) == 0){
                printf("[ctrlz()→ Señal 20 (SIGTSTP) enviada a %d (%s) por %d (%s)]\n",jobs_list[0].pid,jobs_list[0].command_line,getpid(),g_argv);
                jobs_list_add(jobs_list[0].pid,'D',jobs_list[0].command_line);
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