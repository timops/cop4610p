/*
 *
 * producer-consumer.c
 * Tim Green
 * 3/23/14
 * version 1.0
 *
 * Project 3, Part 2: Producer-Consumer
 *
 * producer-consumer.x <sleep time> <num producer threads> <num consumer threads>
 *
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <pthread.h>
#include <semaphore.h>

#include "buffer.h"

buffer_item buffer[BUFFER_SIZE];

int insert_item(buffer_item item);
int remove_item(buffer_item *item);
void *producer(void *param);
void *consumer(void *param);
void buffer_init(void);

pthread_mutex_t mutex;
sem_t s_empty, s_full;

// critical section data for storing current position to read/write in circular queue
int in = 0, out = 0;

int main(int argc, char** argv)
{
  int i;

  // program only functions correctly with 3 arguments, print some help if misused
  if (argc < 4)
  {
    printf("\nHelp:\n");
    printf("producer-consumer.x <sleep time> <num producer threads> <num consumer threads>\n\n");
    exit(0);
  }

  // convert string arguments to integers for later use
  const int SLEEP = atoi(argv[1]);
  const int PRODUCER = atoi(argv[2]);
  const int CONSUMER = atoi(argv[3]);

  // create PRODUCER*CONSUMER threads
  pthread_t p_tid[PRODUCER];
  pthread_t c_tid[CONSUMER];

  for (i=0; i<PRODUCER; i++)
  {
    int rc = pthread_create(&p_tid[i], NULL, producer, NULL);
    if (rc) 
    {
      perror("[ERROR]");
      exit(EXIT_FAILURE);
    }
  }

  for (i=0; i<CONSUMER; i++)
  {
    int rc = pthread_create(&c_tid[i], NULL, consumer, NULL);
    if (rc) 
    {
      perror("[ERROR]");
      exit(EXIT_FAILURE);
    }
  }

  // create semaphores and mutex, initializing the semaphores according to BUFFER_SIZE
  buffer_init();

  // exit the main program after SLEEP seconds
  sleep(SLEEP);

  return 0;
}

/*
 *
 * This function initializes the mutex and semaphores required for bounded buffer.
 *
 */
void buffer_init(void)
{
  // create mutex with default attributes (second parameter)
  pthread_mutex_init(&mutex, NULL);

  // create semaphores and initialize (zero full, max empty).  second param ensures that only 
  // threads belonging to this process can share the semaphore data.
  sem_init(&s_empty, 0, BUFFER_SIZE);
  sem_init(&s_full, 0, 0);
}

/*
 *
 * Thread-safe implementation of bounded buffer insert:
 * Wait until there are empty elements in array, then test if we can write using the mutex.
 *
 */
int insert_item(buffer_item item)
{
  int retval = 0;

  retval = sem_wait(&s_empty);
  if (retval)
    return retval;

  retval = pthread_mutex_lock(&mutex);
  if (retval)
    return retval;

  buffer[in] = item;
  in = (in + 1) % BUFFER_SIZE;

  retval = sem_post(&s_full);
  if (retval)
    return retval;

  retval = pthread_mutex_unlock(&mutex);
  if (retval)
    return retval;

  return 0;
}

/*
 *
 * Thread-safe implementation of bounded buffer remove.
 * Wait until there are > 0 elements, get exclusive access with mutex, and remove.
 *
 */
int remove_item(buffer_item *item)
{
  int retval = 0;

  retval = sem_wait(&s_full);
  if (retval)
    return retval;

  retval = pthread_mutex_lock(&mutex);
  if (retval)
    return retval;

  *item = buffer[out];
  out = (out + 1) % BUFFER_SIZE;
  
  retval = sem_post(&s_empty);
  if (retval)
    return retval;

  retval = pthread_mutex_unlock(&mutex);
  if (retval)
    return retval;

  return 0;
}

/*
 *
 * Threaded producer function:
 * Loops until the main program exits, producing random numbers for buffer.
 *
 */
void *producer(void *param)
{
  int my_rand;
  
  // quiet the warnings for unused param argument
  (void) param;

  while (1)
  {
    sleep(rand() % 5 + 1);
    my_rand = rand() % RAND_MAX;
    if (insert_item(my_rand))
      printf("[ERROR]: failure in critical section of producer thread!\n");
    else
      printf("Producer produced %d\n", my_rand);
  }
}

/*
 *
 * Threaded consumer function:
 * Loops until the main program exits, consuming random numbers from buffer.
 *
 */
void *consumer(void *param)
{
  int remove_rand;

  // quiet the warnings for unused param argument
  (void) param;

  while (1)
  {
    sleep(rand() % 5 + 1);
    if (remove_item(&remove_rand))
      printf("[ERROR]: failure in critical section of consumer thread!\n");
    else
      printf("Consumer consumed %d\n", remove_rand);
  }
}
