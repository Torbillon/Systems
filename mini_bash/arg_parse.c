#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/wait.h>
#include "arg_parse.h"
#include "builtin.h"


#define MONEY '$'
#define LBRACE '{'
#define RBRACE '}'
#define LPAR '('
#define RPAR ')'
#define END '\0'
#define EPSILON ""
#define ZERO '0'
#define HASH '#'

#define SPCMD "aecho"
#define SIZE 2000000

// deterministic finite automaton
int delta[2][2] = {
  {0,1},
  {1,0}
};

struct stack {
  char* line;
  char* buf;
  int top;
};

typedef struct stack stack;
/* On success, returns index of top of buffer */
int command(int start, stack* stk);
/* On success, returns index of top of buffer */
int action(int start, int end, stack* stk);
/* replace newline with space */
void replaceSpace(char* buf);

char** arg_parse(char* line, int *argcp) {
  int i = 0;
  int len = strlen(line);
  int count = 0;
  int pars = 0;


  int st = 0;
  // Ignore leading spaces
  while(isspace(line[i]) && i < len)
    line[i++] = '\0';

  // If no arguments
  if(i == len)
    return NULL;
  count++;

  /*
   * Compute argc
   */
  while(i < len && !(1-st && line[i] == HASH)) {
    // while line[i] is still an "argument" char
    for(; (!isspace(line[i]) || (isspace(line[i]) && pars%2)) && i < len && !(1-st && line[i] == HASH); i++) {
      st = delta[st][line[i] == '\"'];
      if(line[i] == '\"')
        pars++;
      else
        line[i-pars] = line[i];
    }
    for(; isspace(line[i]) && i < len && !(1-st && line[i] == HASH); i++)
      line[i-pars] = '\0';
    if(i < len && !(1-st && line[i] == HASH))
      count++;
  }

  line[i-pars] = '\0';
  // If odd number of parantheses
  if(pars%2) {
    fprintf(stderr, "Odd number of : \"\n");
    return NULL;
  }


  /*
   * Initialize the argument array
   */
  char** token = malloc((count + 1) * sizeof(char *));
  int j = 0;
  i = 0;
  while(line[i] == '\0')
    i++;

  token[j++] = line + i;
  for(; j < count; j++) {
    for(; line[i] != '\0'; i++) {}
    for(; line[i] == '\0'; i++) {}
    token[j] = line + i;
  }
  token[count] = NULL;

  if(argcp != NULL)
    *argcp = count;
  return token;
}


int expand(char* orig, char* new, int newsize) {
  int i = 0;
  int j = 0;
  stack* stk = malloc(sizeof(stack));

  /* replaces $$ and ${x} with the proccess's pid_t and environment variables, respectively */
  while(orig[i] != '\0') {
    if((orig[i] == MONEY) & (orig[i+1] == LBRACE)) {
      int v = i;
      while(orig[i] != '\0' && orig[i] != RBRACE) {
        i++;
      }
      if(orig[i] == '\0')
        return 0;
      else {
        orig[i] = END;
        char* x = getenv(orig+v+2);
        if(x == NULL)
          x = EPSILON;

        snprintf(new+j,SIZE-j+1,"%s",x);
        while(new[j++] != '\0') {}
        j--;
        orig[i++] = RBRACE;
      }
    } else if((orig[i] == MONEY) & (orig[i+1] == MONEY)) {
      pid_t proc = getpid();
      snprintf(new+j,SIZE-j+1,"%i",proc);

      while(new[j++] != '\0') {}
      j--;
      i += 2;

    } else if((orig[i] == MONEY) && (orig[i+1] == LPAR)) {
      stk->line = orig;
      stk->buf = new;
      stk->top = i + 2;

      int x;
      if((x = command(j,stk)) == 0)
        return 0;

      i = stk->top;
      j = x;
    } else if(orig[i] == RPAR) {
      return 0;
    } else {
      new[j++] = orig[i++];
    }
  }

  new[j] = '\n';
  new[j+1] = '\0';
  free(stk);
  return 1;
}

/* recursive function that recognizes a string of tokens of form: $( cmd )
 * and executes them.
 */
int command(int j, stack* stk) {
  int start = j;
  char* orig = stk->line;
  char* new = stk->buf;
  int i = stk->top;

  while(orig[i] != '\0' && orig[i] != RPAR) {
    if((orig[i] == MONEY) & (orig[i+1] == LBRACE)) {
      int v = i;
      while(orig[i] != '\0' && orig[i] != RBRACE) {
        i++;
      }
      if(orig[i] == '\0')
        return 0;
      else {
        orig[i] = END;
        char* x = getenv(orig+v+2);
        if(x == NULL)
          x = EPSILON;

        snprintf(new+j,SIZE-j+1,"%s",x);
        while(new[j] != '\0') {
          j++;
        }
        orig[i++] = RBRACE;
      }

    } else if((orig[i] == MONEY) & (orig[i+1] == MONEY)) {
      pid_t proc = getpid();
      snprintf(new+j,SIZE-j+1,"%i",proc);

      while(new[j] != '\0') {
        j++;
      }
      i += 2;

    } else if(orig[i] == MONEY && orig[i+1] == LPAR) {
      stk->line = orig;
      stk->buf = new;
      stk->top = i + 2;

      int x;
      if((x = command(j,stk)) == 0)
        return 0;

      i = stk->top;
      j = x;
    } else {
      new[j++] = orig[i++];
    }
  }

  if(orig[i] == '\0')
    return 0;

  // else orig[i] == ')' and execute
  else {
    stk->top = i + 1;
    j = action(start, j, stk);
    return j;
  }
}

/* execute the command starting from index start to end from buffer(stk->buf) and return the top of buffer(stk->buf) */
int action(int start, int end, stack* stk) {
  stk->buf[end] = '\0';
  int i;

  int argc;
  char** token = arg_parse(stk->buf+start, &argc);

  int status;
  if(token == NULL) {
    return start;

  } else if(i = built_in(token,argc)) {
    printf(" ");
    /* If builtin cmd is aecho, redirect STDOUT to pipes[1], and read from pipe[0] */
    if(i == 2) {
      int pipes[2];
      int temp = dup(STDOUT_FILENO);

      if(pipe(pipes) != 0) {
        close(temp);
        return start;
      }

      dup2(pipes[1], STDOUT_FILENO);
      close(pipes[1]);

      fflush(stdout);
      execute_built_in(i,token,argc);

      int n = read(pipes[0],stk->buf + start,SIZE - start + 1);
      stk->buf[start + n - 1] = '\0';
      replaceSpace(stk->buf + start);
      start += n - 1;

      dup2(temp, STDOUT_FILENO);
    } else {
      execute_built_in(i,token,argc);
    }

    return start;
  } else {
    /* Start a new process to do the job. */
    int pipes[2];
    pipe(pipes);
    pid_t cpid = fork();

    if (cpid < 0) {
      perror ("fork");
      return start;

    /* Am child */
    } else if (cpid == 0) {
      close(pipes[0]);
      dup2(pipes[1],STDOUT_FILENO);
      execvp(token[0],token);
      fprintf(stderr,"%s: command not found\n",token[0]);
      exit (127);

    /* Am parent */
    } else {
      close(pipes[1]);
      int n = read(pipes[0],(stk->buf) + start,SIZE - start + 1);

      while(n > 0) {
        stk->buf[start + n - 1] = '\0';
        replaceSpace(stk->buf + start);
        start += n - 1;
        n = read(pipes[0],(stk->buf) + start,SIZE - start + 1);
      }

      /* Have the parent wait for child to complete */
      if(wait(&status) < 0) {
        perror("wait");
      }

      return start;
    }
  }

  free(token);
}

void replaceSpace(char* buf) {
  int i = 0;
  while(buf[i] != '\0') {
    if(buf[i] == '\n')
      buf[i] = ' ';
    i++;
  }
}
