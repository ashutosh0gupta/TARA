#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;

void* add(void *) {
  atomic_fetch_add_explicit( &x, 1, memory_order_release );
  return NULL;
}

void* sub(void *) {
  atomic_fetch_add_explicit( &x, -1, memory_order_relaxed );
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, add, NULL );
  pthread_create(&thr_1, NULL, sub, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);

  assert( x == 0 );
}
