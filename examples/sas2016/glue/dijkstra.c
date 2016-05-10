
#include <assert.h>
#include<pthread.h>
//volatile int sum1, sum2, 
//x1,y1,x2,y2;
  
volatile int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
#define ROUND 1
void fence() {asm("sync");}
int flag1 = 0, flag2 = 0; // N boolean flags 
int turn = 0; // integer variable to hold the ID of the thread whose turn is it
int cs; // variable to test mutual exclusion
//int counter=0;

void* thr1(void * arg) {

    flag1 = 1;
    r2 = turn;
    if(r2 == 1)
    {
	r3 = flag2;
	if(r3 == 0)
    	{	
	    turn = 0;
	    r2 = turn;
	    if(r2 == 0)
    	    {
		flag1=2;
		r3 = flag2;
		if(r3 != 2)
    		{
		    r1 = cs;
	            cs = r1 + 1;
		    flag1 = 0;
	            finished1=1;
    		}
	    }
	}
   }
}


void* thr2(void * arg) { 

    flag2 = 1;
	//fence
    r4 = turn;
    if(r4 == 0)
    {
	r5 = flag1;
	if(r5 == 0)
    	{	
	    turn = 1;
	    r4 = turn;
	    if(r4 == 1)
    	    {
		flag2 = 2;
		r5 = flag1;
		if(r5 != 2)
    		{
		    r6 = cs;
	            cs = r6 + 1;
			//fence
		    flag2 = 0;
	            finished2=1;
    		}
	    }
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

