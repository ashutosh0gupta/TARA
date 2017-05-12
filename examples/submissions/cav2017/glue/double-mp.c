#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int x,y,w,z,v;
int r1, r2, r3, r4;
/* void assume( bool ); */
/* void assert( bool ); */
/* void fence(); */

void* p0(void * arg) {

    x = 1; //fence
    w = 1;
    y = 1;
    z = 1;
    return NULL;
}

void* p1(void * arg) {
     r1 = y;
     r2 = x;
     r3 = z;
     r4 = w;
     return NULL;
}

int main()
{
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( (( r1 != 1 ) || ( r2 == 1)) && (( r3 != 1 ) || ( r4 == 1)) );
}
