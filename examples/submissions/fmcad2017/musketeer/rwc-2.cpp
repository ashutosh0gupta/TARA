#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void assume( bool );
void assert( bool );
void fence();
// int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
int x = 0, y = 0, z = 0, a = 0, b = 0, c = 0;

void* p0(void *) {
  x = 1;
  a = 1;
  return 0;
}

void* p1(void *) {
int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
  r1 = x;
  r2 = y;
  r4 = a;
  r5 = b;
  return 0;
}

void* p2(void *) {
int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
  y = 1;
  b = 1;
  r3 = x;
  r6 = a;
  return 0;
}

int main() {
  __CPROVER_ASYNC_1: p0(0);
  __CPROVER_ASYNC_1: p1(0);
  p2(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_t thr_2;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_create(&thr_2, 0, p2, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  // pthread_join(thr_2, 0);
  // assert(( r1 == 0 || r2 == 1 || r3 == 1) && ( r4 == 0  || r5 == 1 || r6 == 1));
}
