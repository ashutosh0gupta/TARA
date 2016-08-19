#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y=0;
int r1 = 0, r2 = 0, r3 = 0;
void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  x = 2;
  return NULL;
}

void* p1(void *) {
  y = 2;
  return NULL;
}

void* p2(void *) {
  r1 = x;
  y = 1;
  return NULL;
}

void* p3(void *) {
  r3 = y;
  x = 1;
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;
  pthread_t thr_3;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_create(&thr_2, NULL, p2, NULL );
  pthread_create(&thr_3, NULL, p3, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  pthread_join(thr_2, NULL);
  pthread_join(thr_3, NULL);
  assert( !(r1 == 2) || y == 0 || !(r3 == 2) || !(x == 2));
}

