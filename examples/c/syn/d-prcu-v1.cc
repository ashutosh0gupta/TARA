// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html

// Test case emulating simplified D-PRCU.
//
// We emulate an unlink and subsequent free of a node from a list:
// 
//     The variable @x represents the pointer to the node.
//     If x==1, the node is in the list.
//
//     The variable @y represents the node.
//     If y==1, the node is valid.  If y==2, the node has been freed.
//
//
// The client contains two threads:
//
//     r (readers): reads x (pointer to node) and then y (the node).
//
//     w (writer): unlinks node (x=0), waits-for-reader, and then frees
//                 the node (y=2).
//
// The assertion is that the reader cannot access a freed node.
//
#include <atomic>
#include "threads.h"
//#include <assert.h>
#include <stdlib.h>/* //srand, rand */
#include <time.h>       /* time */

#include "librace.h"
#include "mem_op_macros.h"
#include "model-assert.h"

// atomic<int> readers("readers");
// atomic<int> x("x");
// atomic<int> y("y");

atomic_int readers;//("readers");
atomic_int x;//("x");
atomic_int y;//("y");

// int r1, r2;

static void * r(void *obj)
{
    int t1_loop_itr_bnd = 3;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){
        fetch_add(&readers, 1, memory_order_relaxed);

        int r1 = load(&x, memory_order_relaxed);
        if (r1){
            int r2 = load(&y, memory_order_relaxed);
            MODEL_ASSERT(!(r1 == 1 && r2 == 2));
        }

        fetch_add(&readers, -1, memory_order_relaxed);
    }
return NULL;}

static void * w(void *obj)
{
    int t2_loop_itr_bnd = 3;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
        store(&x, 0, memory_order_relaxed); // disconnect node
        
        // wait-for-readers
        while ( load(&readers, memory_order_relaxed) != 0 ) {
            thrd_yield();
        }

        store(&y, 2, memory_order_relaxed); // free
    }
return NULL;}

int main(int argc, char **argv)
{
    thrd_t t1, t2;

    store(&x, 1,std::memory_order_seq_cst);
    store(&y, 1,std::memory_order_seq_cst);
    store(&readers, 0,std::memory_order_seq_cst);

    // printf("Main thread: creating 2 threads\n");
    thrd_create(&t1, (thrd_start_t)&r, NULL);
    thrd_create(&t2, (thrd_start_t)&w, NULL);

    thrd_join(t1);
    thrd_join(t2);

    // MODEL_ASSERT(!(r1 == 1 && r2 == 2));

    // printf("Main thread is finished\n");

    return 0;
}

//###############################################
//!-M c11 -u 1
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9,R#y#_l52_c22) ) 
//#
//~


//###############################################
//!-M c11 -u 2
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9,R#y#_l52_c22) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) 
//#
//~


//###############################################
//!-M c11 -u 3
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u0) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u1) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u4) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u5) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u6) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(U#readers#_l56_c9,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u0) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u0) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u1) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u1) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u10) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u2) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u2) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u4) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u5) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u6) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u7) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u8) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(U#readers#_l56_c9_u0,R#readers#_l68_c17_u9) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u0) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u7) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u1,R#y#_l52_c22_u1) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u0) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#readers#_l82_c5,R#readers#_l68_c17_u3) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9_u0,R#y#_l52_c22_u1) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18) ∧ rf(W#y#_l72_c9,R#y#_l52_c22) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u0) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u0) ) ∨
//#( rf(W#readers#_l82_c5,R#readers#_l68_c17) ∧ rf(W#x#_l80_c5,R#x#_l50_c18_u1) ∧ rf(W#y#_l72_c9,R#y#_l52_c22_u1) ) 
//#
//~

