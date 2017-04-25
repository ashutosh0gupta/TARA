#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;
atomic_int r = 0;

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_relaxed );
  atomic_store_explicit( &x, 2, memory_order_relaxed );
  return NULL;
}

void* p1(void *) {
  atomic_store_explicit( &x, 3, memory_order_relaxed );
  int l = atomic_load_explicit( &x, memory_order_relaxed );
  if( r == 1 )
    assert( l != 1);
  return NULL;
}


void* p2(void *) {
  int s1 = atomic_load_explicit( &x, memory_order_relaxed );
  int s2 = atomic_load_explicit( &x, memory_order_relaxed );
  if( s1 == 2 && s2 == 3 ) {
    r = 1;
  }
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;

  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_create(&thr_2, NULL, p2, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  pthread_join(thr_2, NULL);
}



//###############################################
//!-M c11
//###############################################
//~
//#
//#All traces are good or infeasable.
//~

