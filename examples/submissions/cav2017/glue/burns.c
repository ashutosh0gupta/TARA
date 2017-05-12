#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


int r1,r2,r3,r4,finished1 = 0,finished2 = 0;
int flag1 = 0, flag2 = 0;
int cs;
/* void assume( bool ); */
/* void assert( bool ); */
/* void fence(); */
void* p0(void * arg) {

    flag1 = 1;
    r2 = flag2;
    if(r2 == 0)
    {
	r1 = cs;
	cs = r1 + 1;
	fence();
	flag1 = 0;
	finished1=1;
    }
    return NULL;
}

void* p1(void * arg) { 

    r3 = flag1;
    if(r3 == 0)
    {
    	flag2 = 1;
    	r3 = flag1;
    	if(r3 == 0)
    	{
		r4 = cs;
		cs = r4 + 1;
		fence();
		flag2 = 0;
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
