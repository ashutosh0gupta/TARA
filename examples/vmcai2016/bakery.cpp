#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// int r1,r2,r3,r4,r5,r6;
int r1; //,r5;//,r6;

int turn = 0;
int flag0 = 0, flag1 = 0;
int done0 = 0, done1 = 0;
void assume( bool );
void assert( bool );
void fence();

int n1 = 0,n2 = 0;
int cs; // variable to test mutual exclusion

void* p0(void *) {
  int r3;
    flag0 = 1;
    r1 = n1;
    int r2 = n2;
    if(r2 > r1)
    {
	r3 = r2 + 1;
    }
    else
    {
	r3 = r1 + 1;
    }
    n1 = r3;
    flag0 = 0;
    if(flag1 == 0)
    {
	r1 = n1;
        r2 = n2;
	if((r2 == 0) || !(r2 < r1))
	{
		cs = cs + 1;
	        n1 = 0;
	        done0 = 1;
	}
    }
    return NULL;
}


void* p1(void *) {
    flag1 = 1;
    int r4 = n1;
    int r5 = n2;
    int r6;
    if(r4 > r5)
    {
	r6 = r4 + 1;
    }
    else
    {
	r6 = r5 + 1;
    }
    n2 = r6;
    flag1 = 0;
    if(flag0 == 0)
    {
	r4 = n1;
        r5 = n2;
	if((r4 == 0) || (r5 < r1))
	{
	        cs = cs + 1;
	        n2 = 0;
	        done1 = 1;
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
  assert(!((done0 == 1) && (done1 == 1)) || cs == 2);
}

