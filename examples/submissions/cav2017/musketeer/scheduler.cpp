#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int current = 1, Pone = 0, Ptwo = 1;

void assume( bool );
void assert( bool );
void fence();

void* p0(void * arg) {
  int r0,r1,r2;
    r0 = Pone;
    r1 = Ptwo;
    if(!((r0 == 0) && (r1 == 0)))
    {
	r2 = current;
	if(r2 == 1)
        {
	     Pone = 1;
	     Ptwo = 0;
	     current = 2;
	}
    }
    return NULL;
}

void* p1(void * arg) {
int s0,s1,s2;
    s0 = Pone;
    s1 = Ptwo;
    if(!((s0 == 0) && (s1 == 0)))
    {
	s2 = current;
	if(s2 == 2)
        {
	     Pone = 0;
	     Ptwo = 1;
	     current = 1;
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
  assert((Pone == 1 && Ptwo == 0) || (Pone == 0 && Ptwo == 1) );
}

