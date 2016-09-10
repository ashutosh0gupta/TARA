// this example is for testing all the reasoning related to branching.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int turn;
int flag0 = 0, flag1 = 0;
int cs = 0;
int done0 = 0, done1 = 0;
void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  flag0 = 1;
  turn = 1;
  if( !((flag1 == 1) && (turn == 1)))
  {
    //busy wait
  //critical section
    cs = cs + 1;
  //critical section ends
    flag0 = 0;
    done0 = 1;
  }
  return NULL;
}

void* p1(void *) {
  flag1 = 1;
  turn = 0;
  if(!((flag0 == 1) && (turn == 0)))
  {   //busy wait
  //critical section
    cs = cs + 1;
  //critical section ends
    flag1 = 0;
    done1 = 1;
  }
  return NULL;
}
int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert(!((done0 == 1) && (done1 == 1)) || cs == 2 );
}

