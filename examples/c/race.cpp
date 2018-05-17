#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();
void fence_arm8_dmb_ld();

atomic_int x = 0, y = 0;

int z;
// void* p0(void *) {
//   atomic_store_explicit( &x, 1, memory_order_release );
//   return NULL;
// }

void* p0(void *) {
  z = 2;
  return NULL;
}

void* p1(void *) {
  z = 0;
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


