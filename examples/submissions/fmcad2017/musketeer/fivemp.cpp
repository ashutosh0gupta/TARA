#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int r1,r2,r3,r4,r5,r6,r7,r8,r9,r10;
int m1, m2, m3, m4, m5, s1, s2, s3, s4, s5;
void assume( bool );
void assert( bool );
void fence();

void* p0(void * arg) {

    m1 = 1;
    m2 = 1;
    m3 = 1;
    m4 = 1;
    m5 = 1;
    s1 = 1;
    s2 = 1;
    s3 = 1;
    s4 = 1;
    s5 = 1;
    return 0;
}

void* p1(void * arg) {
     r1 = s1;
     r2 = m1;
     r3 = s2;
     r4 = m2;
     r5 = s3;
     r6 = m3;
     r7 = s4;
     r8 = m4;
     r9 = s1;
     r10 = m1;
     return 0;
}

int main()
{
  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  // assert( (!( r1 == 1) || (r2 == 1)) && (!( r3 == 1) || (r4 == 1)) && (!( r5 == 1) || (r6 == 1)) && (!( r7 == 1) || (r8 == 1)) && (!( r9 == 1) || (r10 == 1)));
}
