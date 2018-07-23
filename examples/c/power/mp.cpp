#include "power.h"

int x = 0;
int y = 0;


void* p0(void *) {
  x = 1;
  y = 1;
  return NULL;
}

void* p1(void *) {
  int r1 = y;
  if( r1 == 1 ) {
    power_isync();
    int r2 = x;
    assert( r2 == 1);
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
}


