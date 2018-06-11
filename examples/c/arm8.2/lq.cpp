#include "arm.h"

arm_atomic_int x = 0;
arm_atomic_int y = 0;
arm_atomic_int r1 = 0;
arm_atomic_int r2 = 0;

void* p0(void *) {
  int t1 = exhange( &x, 1);
  
  // storeL(&y, 1);
  // int t1 = loadA(&x);
  // store(&r1, t1);
  return NULL;
}

void* p1(void *) {
  int t1 = exhange( &x, 1);

  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( r1 == 1 || r2 == 1 );
}


//###############################################
//!-M arm8.2
//###############################################
//~
//#
//#All traces are good or infeasable.
//~

