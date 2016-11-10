#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;
atomic_int y = 0;
atomic_int r1 = 0;
atomic_int r2 = 0;

// int x = 0, y=0;
// int r1 = 0, r2=0;

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_relaxed );
  r1 = atomic_load_explicit( &y, memory_order_acquire );
  return NULL;
}

void* p1(void *) {
  atomic_store_explicit( &y, 1, memory_order_relaxed );
  r2 = atomic_load_explicit( &x, memory_order_acquire );
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
