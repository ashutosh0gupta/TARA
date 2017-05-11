#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void assume( bool );
void assert( bool );
void fence();
int r1, r2, r3, r4,r5,r6,r7,r8,r9,r10;
int x1,x2,x3,x4,x5,x6,x7,x8,x9,x10;
void* p0(void * arg) {
    x1 = 1;
    x2 = 1;
    x3 = 1;
    x4 = 1;
    x5 = 1;
    x6 = 1;
    x7 = 1;
    x8 = 1;
    x9 = 1;
    x10 = 1;
    return NULL;
}

void* p1(void * arg) {
     r2 = x2;
     r1 = x1;
     r4 = x4;
     r3 = x3;
     r6 = x6;
     r5 = x5;
     r8 = x8;
     r7 = x7;
     r10 = x10;
     r9 = x9;
     return NULL;
}

int main() {
  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, NULL, p0, NULL );
  // pthread_create(&thr_1, NULL, p1, NULL );
  // pthread_join(thr_0, NULL);
  // pthread_join(thr_1, NULL);
  assert((!( r2 == 1) || ( r1 == 1) ) && (!( r4 == 1) || ( r3 == 1)) && (!( r6 == 1) || ( r5 == 1)) && (!( r8 == 1) || ( r7 == 1)) && (!( r10 == 1) || ( r9 == 1)));
}
