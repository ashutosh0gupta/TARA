#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;
atomic_int y = 0;
atomic_int z = 0;

void* p0(void *) {
  int s1 = atomic_load_explicit( &x, memory_order_acquire );
  if( s1 == 1 ) {
    atomic_store_explicit( &y, 1, memory_order_relaxed );
    assert( false );
  }
  return NULL;
}

void* p1(void *) {
  int s1 = atomic_load_explicit( &y, memory_order_relaxed );
  if( s1 == 1 ) {
    atomic_store_explicit( &z, 1, memory_order_relaxed );
  }
  return NULL;
}

void* p2(void *) {
  int s1 = atomic_load_explicit( &z, memory_order_relaxed );
  if( s1 == 1 ) {
    atomic_store_explicit( &x, 1, memory_order_release );
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
//#Final result
//#Bad DNF
//#( hb(W#x#_l34_c5,R#x#_l15_c12) ∧ rf(W#y#_l17_c5,R#y#_l24_c12) ∧ rf(W#z#_l26_c5,R#z#_l32_c12) ) 
//#
//~

