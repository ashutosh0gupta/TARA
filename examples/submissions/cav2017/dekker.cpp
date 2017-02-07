#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

int done0 = 0,done1 = 0;
void assume( bool );
void assert( bool );
void fence();
int flag0 = 0, flag1 = 0;
int turn = 0;
int cs;

void* p0(void * arg) {
    int r1;
    flag0 = 1;
    r1 = flag1;
    if(r1 != 1)
      {
	cs = cs + 1;
	turn = 1;
	flag0 = 0;
	done0 = 1;
    }
    return NULL;
}

void* p1(void * arg) {
    int r2;
    flag1 = 1;
    r2 = flag0;
    if(r2 != 1)
    {
       	cs = cs + 1;
	turn = 0;
	flag1 = 0;
	done1 = 1;
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
