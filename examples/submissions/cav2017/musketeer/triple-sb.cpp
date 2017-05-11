#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y = 0, w = 0, z = 0, a = 0, b = 0;
int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  x = 1;
  w = 1;
  a = 1;
  r1 = y;
  r3 = z;
  r5 = b;
  return 0;
}

void* p1(void *) {
  y = 1;
  z = 1;
  b = 1;
  r2 = x;
  r4 = w;
  r6 = a;
  return 0;
}


int main() {
  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );

  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  assert( ( r1 == 1 || r2 == 1) && ( r3 == 1 || r4 == 1 ) && ( r5 == 1 || r6 == 1 ));
}
