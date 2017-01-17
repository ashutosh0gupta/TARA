#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int m1,m2 = 0;
atomic_int s1,s2 = 0;

void* p0(void *) {
  atomic_store_explicit( &m1, 1, memory_order_relaxed );
  atomic_store_explicit( &m2, 1, memory_order_relaxed );
  // atomic_thread_fence(memory_order_release);
  atomic_store_explicit( &s1, 1, memory_order_relaxed );
  atomic_store_explicit( &s2, 1, memory_order_relaxed );
  return NULL;
}

void* p1(void *) {
  int r1 = atomic_load_explicit( &s1, memory_order_relaxed );
  if( r1 == 1 ) {
    int r2 = atomic_load_explicit( &m1, memory_order_relaxed );
    assert( r2 == 1);
  }
  int r3 = atomic_load_explicit( &s2, memory_order_relaxed );
  if( r3 == 1 ) {
    int r4 = atomic_load_explicit( &m2, memory_order_relaxed );
    assert( r4 == 1);
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
//!-M c11
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( rf(W#s1#_l17_c3,R#s1#_l23_c12) ∧ rf(pre#the_launcher,R#m1#_l25_c14) ) ∨
//#( rf(W#s2#_l18_c3,R#s2#_l28_c12) ∧ rf(pre#the_launcher,R#m2#_l30_c14) ) 
//#
//~

