#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


volatile int r1,r2,r3,r4,r5,r6,r7,r8,r9,finished1 = 0,finished2 = 0,finished3 = 0;
int flag1 = 0, flag2 = 0, flag3 = 0;
int cs;
void assume( bool );
void assert( bool );
void fence();
void* p0(void * arg) {

    flag1 = 1;
	//fence
    r2 = flag2;
    r3 = flag3;
    if(r2 == 0 && r3 == 0)
    {
	r1 = cs;
	cs = r1 + 1;
	finished1=1;
    }
   return 0;
}

void* p1(void * arg) { 

  if(flag1 != 1)
  {
    flag2 = 1;
    r4 = flag1;
    if(r4 != 1)
    {
       r5 = flag3;
       if(r5 != 1)
       {
          r6 = cs;
	  cs = r6 + 1;
          flag2 = 0;
	  finished2=1;
    	}
    }
  }
  return 0;
}

void* p2(void * arg) { 

  if(flag1 != 1)
  {
    r7 = flag1;
    r8 = flag2;
    if(r7 == 0 && r8 == 0)
    {
      flag3 = 1;
	//fence
       r7 = flag1;
       r8 = flag2;
       if(r7 == 0 && r8 == 0)
       {
          r9 = cs;
	  cs = r9 + 1;
          flag2 = 0;
	  finished3=1;
    	}
     }
  }
  return 0;
}

int main() {
  __CPROVER_ASYNC_1: p0(0);
  __CPROVER_ASYNC_1: p1(0);
  p2(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_t thr_2;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_create(&thr_2, 0, p2, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  // pthread_join(thr_2, 0);
  assert(!((finished1 == 1) && (finished2 == 1) && (finished3 == 1)) || cs == 2);
}
