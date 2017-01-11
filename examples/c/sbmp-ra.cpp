#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0, y = 0, z = 0;
atomic_int r1 = 0, r2 = 0, r3 = 0;

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_release );
  r1 = atomic_load_explicit( &y, memory_order_acquire );
  return NULL;
}

void* p1(void *) {
  atomic_store_explicit( &y, 1, memory_order_release );
  atomic_store_explicit( &z, 1, memory_order_release );
  return NULL;
}

void* p2(void *) {
  r2 = atomic_load_explicit( &z, memory_order_acquire );
  r3 = atomic_load_explicit( &x, memory_order_acquire );
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

  assert( r2 == 0 || r1 == 1 || r3 == 1 );
}


//###############################################
//!-M c11
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( !hb(W#x#_l14_c3,R#x#_l27_c8) ∧ !hb(W#y#_l20_c3,R#y#_l15_c8) ∧ hb(W#z#_l21_c3,R#z#_l26_c8) ) 
//#
//~

