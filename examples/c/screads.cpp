//Example source:
// Common Compiler Optimisations are Invalid in the C11 Memory Model and
// what we can do about it. Viktor Vafeiadis, Thibaut Balabonski,
// Soham Chakraborty, Robin Morisset, Francesco Zappa Nardelli, POPL 2015.

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int x = 0;
atomic_int y = 0;
atomic_int r = 0;

void* p0(void *) {
  atomic_store_explicit( &x, 1, memory_order_relaxed );  //atomic_store_explicit( &x, 1, memory_order_release );
  atomic_store_explicit( &x, 2, memory_order_seq_cst );
  atomic_store_explicit( &y, 1, memory_order_seq_cst );
  return NULL;
}

void* p1(void *) {
  atomic_store_explicit( &x, 3, memory_order_seq_cst );
  // atomic_store_explicit( &x, 3, memory_order_relaxed );
  atomic_store_explicit( &y, 2, memory_order_seq_cst );
  return NULL;
}

void* p2(void *) {
  atomic_store_explicit( &y, 3, memory_order_seq_cst );
  int l = atomic_load_explicit( &x, memory_order_seq_cst );
  if( r == 1 )
    assert( l != 1);
  return NULL;
}


void* p3(void *) {
  int s1 = atomic_load_explicit( &x, memory_order_relaxed );
  int s2 = atomic_load_explicit( &x, memory_order_relaxed );
  int s3 = atomic_load_explicit( &x, memory_order_relaxed );
  int t1 = atomic_load_explicit( &y, memory_order_relaxed );
  int t2 = atomic_load_explicit( &y, memory_order_relaxed );
  int t3 = atomic_load_explicit( &y, memory_order_relaxed );
  if(    s1 == 1 && s2 == 2 && s3 == 3
      && t1 == 1 && t2 == 2 && t3 == 3 ) {
    r = 1;
  }
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
}

//###############################################
//!-M c11
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( hb(W#r#_l51_c7,R#r#_l36_c7) ∧ rf(W#x#_l20_c3,R#x#_l35_c11) ∧ rf(W#x#_l20_c3,R#x#_l43_c12) ∧ rf(W#x#_l21_c3,R#x#_l44_c12) ∧ rf(W#x#_l27_c3,R#x#_l45_c12) ∧ rf(W#y#_l22_c3,R#y#_l46_c12) ∧ rf(W#y#_l29_c3,R#y#_l47_c12) ∧ rf(W#y#_l34_c3,R#y#_l48_c12) ) 
//#
//~

