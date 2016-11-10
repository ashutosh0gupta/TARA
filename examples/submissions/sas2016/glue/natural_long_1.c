
#include <assert.h>
#include<pthread.h>
//volatile int sum1, sum2, 
//x1,y1,x2,y2;
  
volatile int x1=0,x2=0,x3=0,x4=0,x5=0,x6=0,x7=0,x8=0,x9=0,x10=0;
int r1, r2, r3, r4,r5,r6,r7,r8,r9,r10; 
#define ROUND 1
void fence() {asm("sync");}

void* thr1(void * arg) {

    x1 = 1; //fence
    x2 = 1;
    x3 = 1;
    x4 = 1;
    x5 = 1;
    x6 = 1;
    x7 = 1;
    x8 = 1;
    x9 = 1;
    x10 = 1;
}

void* thr2(void * arg) { 
     	
     
     r10 = x10;
     r9 = x9;
     r8 = x8;
     r7 = x7;
     r6 = x6;
     r5 = x5;
     r4 = x4;
     r3 = x3;	
     r2 = x2;
     r1 = x1;
     
     

    
}

int main()
{
    pthread_t pt1;
     pthread_create(&pt1,0,thr1,0);
      thr2(0);
      pthread_join(pt1,0);
     //assert(counter==(ROUND<<1));
     //assert(sum1+sum2>=0);
	assert((r1 == 1) || (r10 == 0));
//  __CPROVER_ASYNC_1: thr1(0);
//  thr2(0);
}

