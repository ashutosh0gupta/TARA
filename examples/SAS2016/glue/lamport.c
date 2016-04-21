
#include <assert.h>
#include<pthread.h>
//volatile int sum1, sum2, 
//x1,y1,x2,y2;
  
volatile int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
#define ROUND 1
void fence() {asm("sync");}
int b1 = 0, b2 = 0, x, y; // N boolean flags 
//int turn = 0; // integer variable to hold the ID of the thread whose turn is it
int cs=0; // variable to test mutual exclusion
//int counter=0;

void* thr1(void * arg) {

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
}

void* thr2(void * arg) { 

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

