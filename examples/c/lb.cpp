#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y=0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  int r1 = x;
  if( r1 == 1 )
    y = 1;
  return NULL;
}

void* p1(void *) {
  int r2 = y;
  if( r2 == 1 )
    x = 1;
  return NULL;
}


int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );

  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( x == 0 || y == 0 );
}


//###############################################
//!-M sc
//###############################################
//~
//#
//#All traces are good or infeasable.
//~


//###############################################
//!-M tso
//###############################################
//~
//#
//#All traces are good or infeasable.
//~


//###############################################
//!-M pso
//###############################################
//~
//#
//#All traces are good or infeasable.
//~


//###############################################
//!-M rmo
//###############################################
//~
//#
//#All traces are good or infeasable.
//~


//###############################################
//!-M alpha
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( hb(W#x#_l21_c7,R#x#_l12_c12) ∧ hb(W#x#_l21_c7,R#x#_l34_c11) ∧ hb(W#y#_l14_c7,R#y#_l19_c12) ∧ hb(W#y#_l14_c7,R#y#_l34_c21) ) 
//#
//~

