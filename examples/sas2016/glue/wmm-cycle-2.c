
#include <assert.h>
#include<pthread.h>
//volatile int sum1, sum2, 
//x1,y1,x2,y2;
  
volatile int x,y,w,z,v;
int r1, r2, r3, r4;
#define ROUND 1
void fence() {asm("sync");}

void* thr1(void * arg) {

    x = 1; //fence
    y = 1;
    w = 1;
    z = 1;
}

void* thr2(void * arg) { 
 
     r1 = y;
     r2 = x;
     r3 = z;
     r4 = w;	
    
}

int main()
{
    pthread_t pt1;
     pthread_create(&pt1,0,thr1,0);
      thr2(0);
      pthread_join(pt1,0);
     //assert(counter==(ROUND<<1));
     //assert(sum1+sum2>=0);
assert( (( r1 != 1 ) || ( r2 == 1)) && (( r3 != 1 ) || ( r4 == 1)) );
//  __CPROVER_ASYNC_1: thr1(0);
//  thr2(0);
}

