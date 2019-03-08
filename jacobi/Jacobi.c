#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

// GET RID OF FOR FINAL VERSION
#include <time.h>

#define LEN 1024
#define EPSILON 0.00001
#define SHIFT 1

typedef struct arg_st {
  int thno;
} arg_t;

double* grid[LEN];
double* newg[LEN];
double* delta;
int noth;


/* for barrier */
int increasing = 1;
int count = 0;
sem_t* sem;
sem_t s;



void* jacobi(void* ptr);
int converge();
double max(double x, double y);
void init(char* file);
void uninit();
void barrier();
void printmtx();


int main(int argc, char* argv[]) {
  if(argc != 3) {
    return -1;
  }

  init(argv[1]);
  /* potentially check for atoi() error */
  noth = atoi(argv[2]);
  delta = malloc(sizeof(double)*noth);
  pthread_t thd[noth];
  arg_t args[noth];


  void* unused;
  sem_init(&s,0,1);
  sem = &s;
  /*------------*/
  /* BEGIN TIME */
  /*------------*/
  // clock_t begin = clock();

  int i;
  for(i = 0; i < noth; i++) {
    args[i].thno = i;
    if(pthread_create(&thd[i],NULL,&jacobi,(void *)&args[i])) {
      perror("pthread_create");
      return -1;
    }
  }

  for(i = 0; i < noth; i++)
    pthread_join(thd[i], &unused);

  /*----------*/
  /* END TIME */
  /*----------*/
  // clock_t end = clock();
  // double alpha = (double)(end-begin)/CLOCKS_PER_SEC;
  // printf("%lf\n",alpha);

  printmtx();
  uninit();
  return 0;
}

void* jacobi(void* ptr) {
  arg_t* arg = (arg_t *) ptr;
  int thno = arg->thno;
  int i;
  int j;
  int start = (LEN-2)*thno/noth + SHIFT;
  int end = (LEN-2)*(thno+1)/noth + SHIFT;

  while(1) {

    for(i = start; i < end; i++)
      for(j = 1; j < LEN - 1; j++) {
        newg[i][j]= (grid[i+1][j] + grid[i-1][j] + grid[i][j+1] + grid[i][j-1])/4.0;
      }

    delta[thno] = 0.0;
    for(i = start; i < end; i++)
      for(j = 1; j < LEN - 1; j++) {
        delta[thno] = max(delta[thno], fabs(newg[i][j] - grid[i][j]));
      }

    barrier();
    if(converge())
      break;

    for(i = start; i < end; i++)
      for(j = 1; j < LEN - 1; j++)
        grid[i][j] = (newg[i+1][j] + newg[i-1][j] + newg[i][j+1] + newg[i][j-1])/4.0;

    barrier();
  }
}

/* implementation of barrier using lock(binary semaphore) */
void barrier() {
  // <await(increasing) {++count;}>
  sem_wait(sem);
  while(!increasing) {
    sem_post(sem);
    int i = 0;
    for(; i < 10; i++) {}
    sem_wait(sem);
  }
  ++count;
  sem_post(sem);


  // <await(count == noth || !increasing) {...}>
  sem_wait(sem);
  while(count != noth && increasing) {
    sem_post(sem);
    int i;
    for(i = 0; i < 10; i++) {}
    sem_wait(sem);
  }


  if(count == noth)
    increasing = 0;

  --count;
  if(count == 0)
    increasing = 1;

  sem_post(sem);
}



int converge() {
  int i;
  for(i = 0; i < noth; i++)
    if(delta[i] > EPSILON)
      break;

  return i == noth;
}


double max(double x, double y) {
  if(x > y)
    return x;
  return y;
}

/* intialize og matrix */
void init(char* file) {
  int i;
  int j;

  for(i = 0; i < LEN; i++) {
    grid[i] = malloc(sizeof(double)*LEN);
    newg[i] = malloc(sizeof(double)*LEN);
  }


  FILE* fp = fopen(file,"r");
  if(fp == NULL) {
    perror("Could not open file");
    exit(1);
  }


  for(i = 0; i < LEN; i++)
    for(j = 0; j < LEN; j++) {
      fscanf(fp,"%lf",&grid[i][j]);
      newg[i][j] = grid[i][j];
    }
}

/* clean up this mess */
void uninit() {
  int i;
  for(i = 0; i < LEN; i++) {
    free(grid[i]);
    free(newg[i]);
  }

  free(delta);
  sem_destroy(sem);
}

void printmtx() {
  int i;
  int j;
  for(i = 0; i < LEN; i++) {
    for(j = 0; j < LEN-1; j++)
      printf("%lf ",newg[i][j]);
    printf("%lf\n",newg[i][j]);
  }
}
