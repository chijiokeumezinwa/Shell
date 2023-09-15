#include <stdio.h>
#define SH_RL_BUFSIZE 1024
#define SH_TOKEN_BUFSIZE 64
#define SH_TOKEN_DELIM " \t\r\n\a"

/*
function declarations for builtin shell commands:
*/
int sh_cd(char **args);
int sh_help(char **args);
int sh_exit(char **args);

/*
list of builtin commands
*/

char *builtin_str[] = {
    "cd", "help", "exit"
};

int (*builtin_func[]) (char**) = {
    &sh_cd, &sh_help, &sh_exit
};


int sh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

/*
builtin function implementations
*/
int sh_cd(char **args){
    if(args[1] == NULL){
        fprintf(stderr, "sh: expected argument to \"cd\"\n");
    }
    else{
        if(chdir(args[1]) != 0){
            perror("sh");
        }
    }
    return 1;
}

int sh_help(char **args){
    int i;
    printf("Chijioke Umezinwa's shell\n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for(i = 0; i < sh_num_builtins(); i++){
        printf("  %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int sh_exit(char **args){
    return 0;
}
int main(int argc, char* argv[]){
    //load config files if any

    //run command loop
    sh_loop();

    //perform clean up 
    
    return EXIT_SUCCESS;
}

void sh_loop(){
    char *line;
    char **args;
    int status;

    do{
        printf(">");
        line = sh_read_line();
        args = sh_split_line(line);
        status = sh_execute(args);

        free(line);
        free(args);
    }while(status);
}

//old version
/* char *sh_read_line(void){
    int bufsize = SH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if(!buffer){
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while(1){
        //read a character
        c = getchar();

        //if eof is reached we replace with null character
        if(c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        } else{
            buffer[position] = c;
        }
        position++;
        
        if(position >= bufsize){
            bufsize += SH_RL_BUFSIZE;
            buffer = realloc(buffer,bufsize);

            if(!buffer) {
                fprintf(stderr, "sh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
} */

char *sh_read_line(){
    char *line = NULL;
    ssize_t bufsize = 0;

    //getline will allocate buffer for us

    if(getline(&line, &bufsize, stdin) == -1){
        if(feof(stdin)){
            exit(EXIT_SUCCESS);
        }else{
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **sh_split_line(char *line){
    int bufsize = SH_TOKEN_BUFSIZE;
    int position = 0;

    char ** tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if(!tokens){
        fprintf(stderr, "sh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, SH_TOKEN_DELIM);
    //sequence of calls to strtok breaks up the line
    //into tokens
    while(token != NULL){
        tokens[position] = token;
        position++;

        if(position >= bufsize){
            bufsize += SH_TOKEN_BUFSIZE;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if(!tokens){
                fprintf(stderr, "sh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SH_TOKEN_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

int sh_launch(char **args){
    pid_t pid;
    pid_t wpid;

    int status;
    pid = fork();
    if(pid == 0){
        //child process
        if(execvp(args[0], args) == -1){
            perror("sh");
        }
        exit(EXIT_FAILURE);
    }
    else if (pid < 0){
        //error forking
        perror("sh");
    }
    else{
        //parent process
        do{
            wpid = waitpid(pid, &status, WUNTRACED);
        }while(!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int sh_execute(char **args){
    int i;
    if(args[0] == NULL){
        return 1;
        //an empty command was entered.
    }

    for(i = 0; i < sh_num_builtins(); i++){
        //this tries to find if the command matches
        //any of the builtin commands
        if(strcmp(args[0], builtin_str[i] == 0)) {
            return (*builtin_func[i])(args);
        }
    }

    return sh_launch(args);
}