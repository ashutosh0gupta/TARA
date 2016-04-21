
#include <assert.h>
#include<pthread.h>
//volatile int sum1, sum2, 
//x1,y1,x2,y2;
  
volatile int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
#define ROUND 1
void fence() {asm("sync");}
int c1 = 0, c2 = 0,n1=0,n2=0; // N boolean flags 
//int turn = 0; // integer variable to hold the ID of the thread whose turn is it
int cs; // variable to test mutual exclusion
//int counter=0;

void* thr1(void * arg) {

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
    
}


void* thr2(void * arg) { 

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
	if((r4 == 0) || (r5 < r1))
	{
		r6 = cs;
	        cs = r6 + 1;
	        n2 = 0;
	        finished2=1;
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
	assert(!((finished1 == 1) && (finished2 == 1)) || cs == 2);
//  __CPROVER_ASYNC_1: thr1(0);
//  thr2(0);
}

