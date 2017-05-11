#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int done0 = 0, done1 = 0;
void assume( bool );
void assert( bool );
void fence();
int flag0 = 0, flag1 = 0;
int cs; // variable to test mutual exclusion

void* p0(void * arg) {
    int r1,r2;
    flag0 = 1;
    r2 = flag1;
    if(r2 < 3)
    {
	flag0 = 3;
	r2 = flag1;
	if(r2 != 1)
	{
	    flag0 = 4;
            r1 = cs;
	    cs = r1 + 1;
	    r2 = flag1;
	    if((2 > flag1) || (flag1 < 3) )
	     {
                flag0 = 0;
		done0 = 1;
	     }
         }
     }
    return NULL;
}
void* p1(void * arg) {
    int r3, r4;
    flag1 = 1;
    r3 = flag0;
    if(r3 < 3)
    {
    	flag1 = 3;
    	r3 = flag0;
    	if(r3 != 1)
    	{
		flag1 = 4;
		r3 = flag0;
		if(r3 < 2)
		{
		     r4 = cs;
		     cs = r4 + 1;
                     flag1 = 0;
		     done1 = 1;
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
  assert(!(( done0 == 1) && ( done1 == 1)) || cs == 2);
}
