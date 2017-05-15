#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
/* void assume( bool ); */
/* void assert( bool ); */
/* void fence(); */
int r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0;
int x = 0, y = 0, z = 0, a = 0, b = 0, c = 0;

void* p0(void *) {
  x = 1;
  a = 1;
  return NULL;
}

void* p1(void *) {
  r1 = x;
  r2 = y;
  r4 = a;
  r5 = b;
  return NULL;
}

void* p2(void *) {
  y = 1;
  b = 1;
  r3 = x;
  r6 = a;
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
  assert(( r1 == 0 || r2 == 1 || r3 == 1) && ( r4 == 0  || r5 == 1 || r6 == 1));
}
