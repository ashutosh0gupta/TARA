#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void assume( bool );
void assert( bool );
void fence();

int x1 = 0, x2 = 0, x3 = 0, x4 = 0, x5 = 0;
int z = 0;
void* p0(void *) {
  x1 = 1;
  x2 = 1;
  x3 = 1;
  x4 = 1;
  x5 = 1;
  return 0;
}

void* p1(void *) {
  int t0 = 0;

  if( x4 == 0 )
    if( x4 == 1 )
      if( x2 == 0 )
        if( x2 == 1 )
          if( x3 == 0 )
            if( x3 == 1 )
              if( x5 == 0 )
                if( x5 == 1 )
                  if( x1 == 0 )
                    if( x1 == 1 )
                      z = 1;
  return 0;
}

int main() {

  __CPROVER_ASYNC_1: p0(0);
  p1(0);

  // pthread_t thr_0;
  // pthread_t thr_1;
  // pthread_create(&thr_0, 0, p0, 0 );
  // pthread_create(&thr_1, 0, p1, 0 );
  // pthread_join(thr_0, 0);
  // pthread_join(thr_1, 0);
  assert( z == 0 );
}
