#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>


// int r1,r2,r3,r4,r5,r6,finished1 = 0,finished2 = 0;
void assume( bool );
void assert( bool );
void fence();

atomic_int cs, finished1 = 0,finished2 = 0; // variable to test mutual exclusion
atomic_int c1 = 0,c2 = 0,n1 = 0,n2 = 0;

void* p0(void * arg) {
  int r1,r2,r3;
    atomic_store_explicit(&c1, 1, memory_order_relaxed);
    r1 = atomic_load_explicit(&n1,  memory_order_relaxed);
    r2 = atomic_load_explicit(&n2,  memory_order_relaxed);
    // c1 = 1;
    // r1 = n1;
    // r2 = n2;
    if(r2 > r1)
	r3 = r2 + 1;
    else
	r3 = r1 + 1;
    atomic_store_explicit(&n1, r3, memory_order_relaxed);
    atomic_store_explicit(&c1, 0, memory_order_relaxed);
    // n1 = r3;
    // c1 = 0;
    // if(c2 == 0)
    r3 = atomic_load_explicit(&c2,  memory_order_relaxed);
    if( r3 == 0 )
    {
      r1 = atomic_load_explicit(&n1,  memory_order_relaxed);
      r2 = atomic_load_explicit(&n2,  memory_order_relaxed);
	// r1 = n1;
        // r2 = n2;
	if((r2 == 0) || !(r2 < r1))
	{
          r3 = cs;
          cs = r3 + 1;
          //fence();
          atomic_store_explicit(&n1, 0, memory_order_relaxed);
          // n1 = 0;
          finished1=1;
	}
    }
    return NULL;
}


void* p1(void * arg) { 
  int r6,r5,r4;
    atomic_store_explicit(&c2, 1, memory_order_relaxed);
    r4 = atomic_load_explicit(&n1,  memory_order_relaxed);
    r5 = atomic_load_explicit(&n2,  memory_order_relaxed);
    // c2 = 1;
    // r4 = n1;
    // r5 = n2;
    if(r4 > r5)
    {
	r6 = r4 + 1;
    }
    else
    {
	r6 = r5 + 1;	
    }
    atomic_store_explicit(&n2, r6, memory_order_relaxed);
    atomic_store_explicit(&c2, 0, memory_order_relaxed);
    // n2 = r6;
    // c2 = 0;
    // if(c1 == 0)
    r6 = atomic_load_explicit(&c1,  memory_order_relaxed);
    if( r6 == 0 )
    {
      r4 = atomic_load_explicit(&n1,  memory_order_relaxed);
      r5 = atomic_load_explicit(&n2,  memory_order_relaxed);
	// r4 = n1;
        // r5 = n2;
	if((r4 == 0) || (r5 < r4))
	{
		r6 = cs;
	        cs = r6 + 1;
	        // n2 = 0;
          atomic_store_explicit(&n2, 0, memory_order_relaxed);
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

