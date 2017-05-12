#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
void assume( bool );
void assert( bool );
void fence();

int c1 = 0,c2 = 0,n1 = 0,n2 = 0;
int cs; // variable to test mutual exclusion

void* p0(void * arg) {

    c1 = 1;
    r1 = n1;
    r2 = n2;
    if(r2 > r1)
    {
	r3 = r2 + 1;
    }
    else
    {
	r3 = r1 + 1;	
    }
    n1 = r3;
    c1 = 0;
    if(c2 == 0)
    {
	r1 = n1;
        r2 = n2;
	if((r2 == 0) || !(r2 < r1))
	{
		r3 = cs;
	        cs = r3 + 1;
	        //fence();
	        n1 = 0;
	        finished1=1;
	}
    }
    return 0;
}


void* p1(void * arg) { 

    c2 = 1;
    r4 = n1;
    r5 = n2;
    if(r4 > r5)
    {
	r6 = r4 + 1;
    }
    else
    {
	r6 = r5 + 1;	
    }
    n2 = r6;
    c2 = 0;
    if(c1 == 0)
    {
	r4 = n1;
        r5 = n2;
	if((r4 == 0) || (r5 < r4)) // r1 was here fix
	{
		r6 = cs;
	        cs = r6 + 1;
	        n2 = 0;
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

