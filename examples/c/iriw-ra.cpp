#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

atomic_int x = 0, y=0;
atomic_int r1 = 0, r2 = 0, r3 = 0,r4 = 0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_release );
  return NULL;
}

void* p1(void *) {
  atomic_store_explicit( &y, 1, memory_order_release );
  return NULL;
}

void* p2(void *) {
  r1 = atomic_load_explicit( &x, memory_order_acquire );
  r2 = atomic_load_explicit( &y, memory_order_acquire );
  return NULL;
}

void* p3(void *) {
  r3 = atomic_load_explicit( &y, memory_order_acquire );
  r4 = atomic_load_explicit( &x, memory_order_acquire );
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;
  pthread_t thr_3;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_create(&thr_2, NULL, p2, NULL );
  pthread_create(&thr_3, NULL, p3, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  pthread_join(thr_2, NULL);
  pthread_join(thr_3, NULL);
  assert( r1 == 0 || r2 == 1 || r3 == 0 || r4 == 1);
}


//###############################################
//!-M c11
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( !hb(W#x#_l14_c3,R#x#_l31_c8) ∧ !hb(W#y#_l19_c3,R#y#_l25_c8) ∧ hb(W#x#_l14_c3,R#x#_l24_c8) ∧ hb(W#y#_l19_c3,R#y#_l30_c8) ) 
//#
//~

