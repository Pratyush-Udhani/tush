#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define TUSH_RL_BUFSIZE 1024
#define TUSH_TOK_BUFSIZE 64
#define TUSH_TOK_DELIM " \t\r\n\a"

void tush_loop(void);
char *tush_read_line(void);
char **tush_split_line(char *);
int tush_execute(char **);

//builtin functions
int tush_cd(char **args);
int tush_help(char **args);
int tush_exit(char **args);

char *builtin_str[] = {
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &tush_cd,
  &tush_help,
  &tush_exit
};

int tush_num_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}


int main(int argc, char **argv)
{
  // Loading config files

  tush_loop();
 
  return EXIT_SUCCESS;
}

void tush_loop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("$ ");
    line = tush_read_line();
    args = tush_split_line(line);
    status = tush_execute(args);

    free(line);
    free(args);
  } while (status);

}

char *tush_read_line(void) 
{
  int bufsize = TUSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = (char *)malloc(sizeof(char) * bufsize);
  int c;

  if(!buffer) {
    fprintf(stderr, "Memory allocation failed\n");
    exit(EXIT_FAILURE);
  }

  while(1) {
    c = getchar();

    if(c == EOF || c == '\n') {
      //replace the last position with \0
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If the length exceeds bufsize, reallocate memory
    if(position >= bufsize) {
      bufsize += TUSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if(!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **tush_split_line(char *line)
{
  int bufsize = TUSH_TOK_BUFSIZE, position = 0;
  char **tokens = (char **)malloc(sizeof(char *) * bufsize);
  char *token;

  if(!tokens) {
    fprintf(stderr, "memory allocation failed for split line");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, TUSH_TOK_DELIM);
  while(token != NULL) {
    tokens[position] = token;
    position++;

    if(position >= bufsize) {
      bufsize += TUSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if(!tokens) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, TUSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int tush_launch(char **args)
{
  pid_t pid, wpid;
  int status;

  pid = fork();
  if(pid == 0) {
    //child process
    if(execvp(args[0], args) == -1) {
      perror("tush");
      exit(EXIT_FAILURE);
    }
  } else if (pid < 0) {
    perror("tush");
  } else {
    //parent process
    do {
      wpid = waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}


int tush_cd(char** args)
{
  if(args[1] == NULL) {
    fprintf(stderr, "expected arg to cd");
  } else {
    if(chdir(args[1]) != 0) {
      perror("tush");
    }
  }
  return 1;
}

int tush_help(char** args)
{
  int i;
  printf("TUSH\n");
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");
  
  for(i = 0; i < tush_num_builtins(); i++) {
    printf(" %s\n", builtin_str[i]);
  }

  printf("Use the man command for information on other programs.\n");
  return 1;
}

int tush_exit(char **args)
{
  return 0;
}

int tush_execute(char **args)
{
  int i;
  if(args[0] == NULL) {
    return 1;
  }

  for(i = 0; i < tush_num_builtins(); i++) {
    if(strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return tush_launch(args);
}
