#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
void assume( bool );
void assert( bool );
void fence();

int x = 0, y = 0, z = 0, w = 0, s = 0;
int r1 = 0, r2 = 0, r3 = 0, r4 = 0;

void* p0(void *) {
  x = 1;
  y = 1;
  z = 1;
  r1 = 1;
  r2 = 1;
  r3 = 1;
  r4 = 1;
  return NULL;
}

void* p1(void *) {
  if( r1 > x ) {
    if( x == 0 )
      z = 1;
    else if ( y == 0 )
      x = 1;
    else
      w = 1;
  }
  return NULL;

}

void* p2(void *) {
  if( r2 > y ) {
    if( y == 0 )
      x = 1;
    else if( z == 0 )
      w = 1;
    else
      z = 1;
  }
  return NULL;

}

void* p3(void *) {
  if( r3 > z ) {
    if( z == 0 )
      w = 1;
    else if( x == 0 )
      z = 1;
    else 
      x = 1;
  }
  return NULL;
}

void* p4(void *) {
  if( r4 > w ) {
    if( w == 0 )
      y = 1;
    else if( z == 0 )
      x = 1;
    else 
      z = 1;
  }
  return NULL;
}



int main() {
  pthread_t thr_0;
  pthread_t thr_1;
  pthread_t thr_2;
  pthread_t thr_3;
  pthread_t thr_4;
  pthread_create(&thr_0, NULL, p0, NULL );
  pthread_create(&thr_1, NULL, p1, NULL );
  pthread_create(&thr_2, NULL, p2, NULL );
  pthread_create(&thr_3, NULL, p3, NULL );
  pthread_join(thr_0, NULL);
  pthread_create(&thr_4, NULL, p4, NULL );
  pthread_join(thr_1, NULL);
  pthread_join(thr_2, NULL);
  pthread_join(thr_3, NULL);
  pthread_join(thr_4, NULL);
  assert( x == 0 || z == 0 || w == 0 || y == 0 );
}

