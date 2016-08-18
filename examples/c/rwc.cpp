#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y=0;
int r1 = 0, r2 = 0, r3 = 0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  x = 1;
  return NULL;
}

void* p1(void *) {
  r1 = x;
  r2 = y;
  return NULL;
}

void* p2(void *) {
  y = 1;
  r3 = x;
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
  assert( r1 == 0 || r2 == 1 || r3 == 1);
}

