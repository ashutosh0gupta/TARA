#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
void assume( bool );
void assert( bool );
void fence();
int flag1 = 0, flag2 = 0;
int cs;
int turn = 0;
void* p0(void * arg) {

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
   return NULL;
}


void* p1(void * arg) { 

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
  return NULL;
}

int main() {

  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, NULL, p0, NULL );
  // pthread_create(&thr_1, NULL, p1, NULL );
  // pthread_join(thr_0, NULL);
  // pthread_join(thr_1, NULL);
  assert(!((finished1 == 1) && (finished2 == 1)) || cs == 2);
}
