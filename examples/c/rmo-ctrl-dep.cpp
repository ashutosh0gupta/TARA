#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y=0;
int r1 = 0, r2=0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  int t0 = 0;
  r1 = x;
  t0 = r1 + 1;
  if( t0 > 0 )
    y = 1;
  return NULL;
}

void* p1(void *) {
  int t1 = 0;
  r2 = y;
  t1 = r2 + 1;
  if( t1 > 0 )
    x = 1;
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( r1 == 0 || r2 == 0 );
}
