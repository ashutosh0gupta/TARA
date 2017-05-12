#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
/* void assume( bool ); */
/* void assert( bool ); */
/* void fence(); */
int x1 = 0, x2 = 0, x3 = 0;
int z = 0;
void* p0(void *) {
  x1 = 1;
  x2 = 1;
  x3 = 1;
  return NULL;
}

void* p1(void *) {
  int t0 = 0;

  if( x3 == 0 )
    if( x3 == 1 )
      if( x1 == 0 )
        if( x1 == 1 )
          if( x2 == 0 )
            if( x2 == 1 )
              z = 1;
  return NULL;
}

int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_join(thr_0, NULL);
  pthread_join(thr_1, NULL);
  assert( z == 0 );
}
