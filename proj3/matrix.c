/*
 *
 * matrix.c
 * Tim Green
 * 3/20/14
 * version 1.0
 *
 * Project 3, Part 1 - Matrix Multiplication
 *
*/

#define M 3 
#define K 2 
#define N 3

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void *CalcProduct(void *param);

typedef struct v
{
  int i;
  int j;
} Element;

// A[COLS] == B[ROWS]
int A[M][K] = { {1,4}, {2,5}, {3,6} };  // 3x2
int B[K][N] = { {8,7,6}, {5,4,3} };     // 2x3

int C[M][N];

int main(void)
{

  Element E[M][N]; 
  pthread_t tid[M][N];
  pthread_attr_t attr;

  int i, j;

  printf("Matrix A:\n");
  for (i=0; i<M; i++)
  {
    for (j=0; j<K; j++)
      printf("%d ", A[i][j]);

    printf("\n");
  }
  printf("\n");

  printf("Matrix B:\n");
  for (i=0; i<K; i++)
  {
    for (j=0; j<M; j++)
      printf("%d ", B[i][j]);

    printf("\n");
  }
  printf("\n");

  // Explicitly create joinable threads, even though it should be the default.
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (i=0; i<M; i++)
  {
    for (j=0; j<N; j++) 
    {
      E[i][j].i = i;
      E[i][j].j = j;
      int rc = pthread_create(&tid[i][j], &attr, CalcProduct, (void *) &E[i][j]);
      if (rc) 
      {
        perror("[ERROR]");
        exit(EXIT_FAILURE);
      }
    }
  }

  pthread_attr_destroy(&attr);

  for (i=0; i<M; i++) 
  {
    for (j=0; j<N; j++) 
    {
      int rc = pthread_join(tid[i][j], NULL);
      if (rc)
      {
        printf("[ERROR]: return code from pthread_join is %d", rc);
        exit(EXIT_FAILURE);
      }
    }
  }

  printf("Matrix C = AB\n");
  for (i=0; i<M; i++)
  {
    for (j=0; j<N; j++)
      printf("%d ", C[i][j]);

    printf("\n");
  }
  printf("\n");

  pthread_exit(NULL);
}

/*
 *
 *  This function builds a sum of i products.  Take element i of row j from Matrix A,
 *  and multiply it by element i of column k from Matrix B.  The result is stored in a
 *  global data structure so no value is returned.
 *
*/
void *CalcProduct(void *param)
{
  Element *row_col = (Element *) param;
  int i;

  const int ROW = row_col->i;
  const int COL = row_col->j;

  C[ROW][COL] = 0;

  for (i=0; i<K; i++)
    C[ROW][COL] += (A[ROW][i] * B[i][COL]);

  pthread_exit(NULL);
}
