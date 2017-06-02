#include "arm.h"

arm_atomic_int x = 0, y = 0;

// void* p0(void *) {
//   atomic_store_explicit( &x, 1, memory_order_release );
//   return NULL;
// }

void* p0(void *) {
  // store(&x , 1);
  int s1 = exchange(&x, 1 );
  // int s2 = fetch_add( &x, 1 );
  fence();
  // fence_arm_dmb_ld();
  store( &y, 1);
  return NULL;
}

void* p1(void *) {
  int s1 = load( &y );
  fence_arm_dmb_ld();
  int s2 = load( &x );
  assert( s1 != 1 || s2 == 1 );
  // assert( y != 1 || x != 0 );
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;

  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  // pthread_create(&thr_2, NULL, p2, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  // pthread_join(thr_2, NULL);
}


