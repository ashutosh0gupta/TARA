#include "arm.h"

arm_atomic_int x = 0;
arm_atomic_int y = 0;

void* p0(void *) {
  store(&x, 1);
  storeL(&y, 1);
  return NULL;
}

void* p1(void *) {
  int r1 = loadA(&y);
  if( r1 == 1 ) {
    int r2 = load(&x);
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


//###############################################
//!-M arm8.2
//###############################################
//~
//#
//#All traces are good or infeasable.
//~

