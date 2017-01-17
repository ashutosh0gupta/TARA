#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;
atomic_int y = 0;

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_relaxed );
  atomic_store_explicit( &y, 1, memory_order_relaxed );
  return NULL;
}

void* p1(void *) {
  int r1 = atomic_load_explicit( &y, memory_order_relaxed );
  if( r1 == 1 ) {
    int r2 = atomic_load_explicit( &x, memory_order_relaxed );
    assert( r2 == 1);
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
//#( rf(W#y#_l15_c3,R#y#_l20_c12) ∧ rf(pre#the_launcher,R#x#_l22_c14) ) 
//#
//~

