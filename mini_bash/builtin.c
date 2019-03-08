#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define FD_OUT stdout
#define SPACE " "
#define NEWLINE "\n"
#define STRFORM "%s"

/* Prototypes for helper functions */
void xit(char** argv, int argc);
void eco(char** argv, int argc);
void chDir(char** argv, int argc);
void setEnv(char** argv, int argc);
void unsetEnv(char** argv, int argc);

struct command {
  char *y;
  void(*z)(char** argv, int argc);
};

const struct command tp[] = {
  {"",NULL},
  {"exit",&xit},
  {"aecho",&eco},
  {"cd",&chDir},
  {"envset",&setEnv},
  {"envunset",&unsetEnv},
  {NULL,NULL}
};
/*
 * this method runs the command based on the argument input
 */
int built_in(char **argv, int argc) {
  int i;
  for(i = 1; tp[i].y != NULL; i++)
    if(!strcmp(tp[i].y,argv[0])) {
      return i;
    }
  return 0;
}
int execute_built_in(int i, char** argv, int argc) {
  tp[i].z(argv, argc);
}
void xit(char** argv, int argc) {
  int t = argc > 1 ? atoi(argv[1]) : 0;
  free(argv);
  exit(t);
}

void eco(char** argv, int argc) {

  int j = (argc > 1 && !strcmp(argv[1],"-n")) ? 2 : 1;
  // flag for '-n' option
  int k = (j == 2)? 0 : 1;
  for(; j < argc -1; j++) {
    fprintf(FD_OUT, STRFORM, argv[j]);
    fprintf(FD_OUT, STRFORM, SPACE);
  }

  /* If there exist actual arguments print the last one*/
  if((!k && argc > 2) || (k && argc > 1))
    fprintf(FD_OUT, STRFORM, argv[j]);

  /* print "\n" based on flag */
  if(k)
    fprintf(FD_OUT, STRFORM, NEWLINE);
}

void chDir(char** argv, int argc) {
  if(argc > 1)
    chdir(argv[1]);
  else
    chdir("../");
}

void setEnv(char** argv, int argc) {
  if(argc == 3) {
    setenv(argv[1],argv[2],1);
  }
}
void unsetEnv(char** argv, int argc) {
  if(argc == 2)
    unsetenv(argv[1]);
}
