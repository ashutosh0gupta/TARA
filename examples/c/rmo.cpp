#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y = 0;
int r1 = 0, r2 = 0, r3 = 0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  int t0, t1 = 0;
  x = 1;
  t1 = x;
  if ( t0 > 0 ) {
    y = t1;
    x = 3; }
  else x = 4;
  y = 2;
  return NULL;
}

void* p1(void *) {
  int t2 = 0;
  r1 = y;
  // x = 2;
  return NULL;
}

void* p2(void *) {
  r2 = x;
  r3 = y;
  return NULL;
}
int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_create(&thr_2, NULL, p2, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  pthread_join(thr_2, NULL);
  assert( r1 == 1 || r2 == 1 || r3 == 0 );
}

