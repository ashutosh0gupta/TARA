#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// volatile int r1,r2,r3,r4,r5,r6;
volatile int finished1 = 0,finished2 = 0;
  
int b1 = 0, b2 = 0, x, y;
int cs=0; // variable to test mutual exclusion
void assume( bool );
void assert( bool );
void fence();
void* p0(void * arg) {
  int r1,r2,r3;
    b1 = 1;
    x = 1; //fence
    r1 = y;
    if(r1 == 0)
    {
	y = 1; //fence
	r2 = x;
        if(r2 == 1)
        {
		r3 = cs;
		cs = r3 + 1; //fence
		y = 0;		
		b1 = 0;
		finished1=1;
         }
     }
   return 0;
}

void* p1(void * arg) { 
  int r4,r5,r6;

    b2 = 1;
    x = 2; //fence
    r4 = y;
    if(r4 == 0)
    {
	y = 2; //fence
	r5 = x;
        if(r5 == 2)
        {
		r6 = cs;
		cs = r6 + 1; //fence
		y = 0;		
		b2 = 0;
		finished2=1;
         }
     }
   return 0;
}

int main() {
  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  assert(!((finished1 == 1) && (finished2 == 1)) || cs == 2);
}
