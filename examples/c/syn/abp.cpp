#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

void assume( bool );
void assert( bool );
void fence();

atomic_int Msg = 0, Ack = 0;

atomic_int lRCnt = 0;
atomic_int lSCnt = 0;

void* p0(void *) {
  int lSSt = 0;
  int lAck = atomic_load_explicit(&Ack,  memory_order_relaxed);
  if( lAck == lSSt ) {
    lSSt = 1 - lSSt;
    lSCnt = lSCnt + 1;
    // atomic_store_explicit(&Msg, lSSt, memory_order_release);
    atomic_store_explicit(&Msg, lSSt, memory_order_relaxed);
    int r = lRCnt;
    int s = lSCnt;
    assert(  (r == s) || (r + 1 == s)  );
  }
  return NULL;
}

void* p1(void *arg) {
  int lRSt = 0;
  // int lMsg = atomic_load_explicit(&Msg,  memory_order_acquire);
  int lMsg = atomic_load_explicit(&Msg,  memory_order_relaxed);
  if( lMsg != lRSt ) {
    lRSt = lMsg;
    lRCnt = lRCnt + 1;
    atomic_store_explicit(&Ack, lRSt, memory_order_relaxed);
    int r = lRCnt;
    int s = lSCnt;
    assert(  (r == s) || (r + 1 == s)  );
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
}
