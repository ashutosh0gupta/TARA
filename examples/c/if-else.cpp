#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y = 0, z = 0;
int r1 = 0, r2 = 0, r3 = 0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  int t0 = 0, t1 = 0, t2 = 0;
  int s = 0, t3 = 0;
  x = 1;
  t0 = x;
  if( t0 > 0 ){
    t1 = t0;
    t3 = t1;
  }else {
    s = y;
    t2 = t0 + s;
    t3 = t2;
  }
  z = t3;
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_join(thr_0, NULL);
}

