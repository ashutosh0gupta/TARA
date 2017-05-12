#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

volatile int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;


int b1 = 0, b2 = 0, x, y;
int cs=0; // variable to test mutual exclusion
void assume( bool );
void assert( bool );
void fence();
void* p0(void * arg) {

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
   return NULL;
}

void* p1(void * arg) { 


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
   return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert(!((finished1 == 1) && (finished2 == 1)) || cs == 2);
}
