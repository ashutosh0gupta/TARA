#include "arm.h"

arm_atomic_int x = 0, y = 0, z = 0;
arm_atomic_int obs = 0;

void* p0(void *) {
  store( &z, 2);
  // fence_arm_dmb_full();
  fence_arm_dmb_st();
  int s1 = exchange(&x, 1 );
  int s2 = load(&x);

  if( s2 == 1 ) {
    fence_arm_dmb_ld();
    int s3 = load(&y);
    store( &obs, s3 );
  }
  return NULL;
}

void* p1(void *) {
  store( &y, 1 );
  fence_arm_dmb_st();
  store( &z, 1 );
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;

  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);

  int s1 = load(&z);
  int s2 = load(&obs);
  if( s1 == 2 )
    assert( s2 == 1 );
}

// Following is the expected bad execution
//
//            Wz2        Wy1
//             |∧        ∧|
//       Rx0   | \mo    / |
//       |\    |  \    /  |
//       | \rmw|st \  /   |st
//       |  V  V    \/    |
//       |   Wx1    /\    |
//rmw;rfi|   :     /  \   |
//       | rfi    /    \  |
//       V :     /fr    \ |
//       Rx1    /        \V
//       |     /         Wz1
//    ld |    /
//       |   /
//       V  /
//       Ry0
//
//
//  Note: rfi is not in hb but rmw;rfi. Therefore, no cycle is formed
//


//###############################################
//!-M arm8.2
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( hb(R#y#_l15_c14,W#y#_l22_c3) ∧ hb(W#z#_l24_c3,W#z#_l7_c3) ) 
//#
//~

