#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int x = 0, y=0;
int r1 = 0, r2=0;

void assume( bool );
void assert( bool );
void fence();

void* p0(void *) {
  int t0 = 0;
  t0 = x;
  if( t0 + 1 > 0 )
    y = 1;
  r1 = t0;
  return NULL;
}

void* p1(void *) {
  int t1 = 0;
  t1 = y;
  if( t1 + 1 > 0 )
    x = 1;
  r2 = t1;
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( r1 == 0 || r2 == 0 );
}

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
//#( hb(W#x#_l25_c7,R#x#_l14_c8) ∧ hb(W#y#_l16_c7,R#y#_l23_c8) ) 
//#
//~

