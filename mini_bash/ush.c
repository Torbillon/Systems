/* CS 352 -- Mini Shell!
 *
 *   Sept 21, 2000,  Phil Nelson
 *   Modified April 8, 2001
 *   Modified January 6, 2003
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

/* Assignment two includes */

#include "arg_parse.h"
#include "builtin.h"
/* Constants */

#define LINELEN 1024
#define EXTRA 16
#define SIZE 2000000

/* Prototypes */

void processline (char *line);
void handler() {
  printf("\n");
}
/* Shell main */
int main (void) {

  signal(SIGINT, handler);
  char buffer[LINELEN];
  int len;

  while(1) {

    /* prompt and get line */
    fprintf (stderr, "%% ");
    if(fgets (buffer, LINELEN, stdin) != buffer)
      break;

    /* Get rid of \n at end of buffer. */
    len = strlen(buffer);
    if (buffer[len-1] == '\n')
      buffer[len-1] = 0;

    /* Run it ... */
    processline (buffer);
  }

  if (!feof(stdin))
  perror ("read");

  return 0;
}


void processline (char *line)
{
    pid_t  cpid;
    int    status;
    int argc;
    int i;

    char buf[SIZE];

    if(expand(line,buf,sizeof(buf)) == 0) {
      fprintf(stderr, "invalid command\n");
      return;
    }
    char** token = arg_parse(buf, &argc);

    if(token == NULL) {
      return;

    // if built_in command
    } else if(i = built_in(token,argc)) {
      execute_built_in(i,token,argc);
    } else {
      /* Start a new process to do the job. */
      cpid = fork();
      if (cpid < 0) {
        perror ("fork");
        return;
      }

      /* Check for who we are! */
      if (cpid == 0) {
        /* We are the child! */
        execvp(token[0],token);
        perror ("exec");
        exit (127);
      }

      /* Have the parent wait for child to complete */
      if (wait (&status) < 0) {
        perror ("wait");
        printf("\n");
      }
    }

    free(token);
}
