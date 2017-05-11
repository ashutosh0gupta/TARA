#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void assume( bool );
void assert( bool );
void fence();

int x = 0, y = 0, z = 0, w = 0, s = 0;
int r1 = 0, r2 = 0, r3 = 0;

void* p0(void *) {
  x = 1;
  y = 1;
  z = 1;
  r1 = 1;
  r2 = 1;
  r3 = 1;
  return 0;
}

void* p1(void *) {
  int t0 = 0;
  if( r1 > 0 )
    if( x == 0 )
      z = 1;
  t0 = y;
  return 0;

}

void* p2(void *) {
  int t0 = 0;
  if( r2 > 0 )
    if( y == 0 )
      w = 1;
  t0 = x;
  return 0;

}

void* p3(void *) {
  int t0 = 0;
  if( r3 > 0 )
    if( z == 0 )
      s = 1;
  t0 = z;
  return 0;

}

int main() {
  __CPROVER_ASYNC_1: p0(0);
  __CPROVER_ASYNC_1: p1(0);
  __CPROVER_ASYNC_1: p2(0);
  p3(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_t thr_2;
  // pthread_t thr_3;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_create(&thr_2, 0, p2, 0 );
  // pthread_create(&thr_3, 0, p3, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  // pthread_join(thr_2, 0);
  // pthread_join(thr_3, 0);
  assert( w == 0 || z == 0 || s == 0 );
}

