
#include <assert.h>
#include<pthread.h>
 
volatile int current = 1, Pone = 0, Ptwo = 1;
#define ROUND 1
void fence() {asm("sync");}
void* thr1(void * arg) {
int r0,r1,r2;

    r0 = Pone;
    r1 = Ptwo;
    if(!((r0 == 0) && (r1 == 0)))
    {
	r2 = current;
	if(r2 == 1)
        {
	     Pone = 1;
	     Ptwo = 0;
	     current = 2;
	}
    }
}

void* thr2(void * arg) { 
int s0,s1,s2;
    s0 = Pone;
    s1 = Ptwo;
    if(!((s0 == 0) && (s1 == 0)))
    {
	s2 = current;
	if(s2 == 2)
        {
	     Pone = 0;
	     Ptwo = 1;
	     current = 1;
	}
    }
}

int main()
{
    pthread_t pt1;
     pthread_create(&pt1,0,thr1,0);
      thr2(0);
      pthread_join(pt1,0);
     //assert(counter==(ROUND<<1));
     //assert(sum1+sum2>=0);
	assert((Pone == 1 && Ptwo == 0) || (Pone == 0 && Ptwo == 1) );
//  __CPROVER_ASYNC_1: thr1(0);
//  thr2(0);
}

