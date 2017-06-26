#include "arm.h"

arm_atomic_int x = 0;
arm_atomic_int y = 0;
arm_atomic_int r1 = 0;
arm_atomic_int r2 = 0;

void* p0(void *) {
  storeL(&y, 1);
  int t1 = loadQ(&x);
  store(&r1, t1);
  return NULL;
}

void* p1(void *) {
  storeL(&x, 1);
  int t2 = loadQ(&y);
  store(&r2, t2);
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
//#Final result
//#Bad DNF
//#( hb(R#x#_l10_c12,W#x#_l16_c3) ∧ hb(R#y#_l17_c12,W#y#_l9_c3) ) 
//#
//~

