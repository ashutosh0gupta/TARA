// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html

// Test case emulating D-PRCU.
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

// #include <stdio.h>
// #include "threads.h"
// #include <stdatomic.h>
// #include <model-assert.h>

// #include "librace.h"

// #include "common.h"

// #include "mem_op_macros.h"

// atomic<int> gate("gate");
// atomic<int> readers[2] = {atomic<int>("readers1"),atomic<int>("readers2")};
// atomic<int> x("x");
// atomic<int> y("y");

atomic_int gate;
atomic_int reader_0;
atomic_int reader_1;
atomic_int x;
atomic_int y;

// int r1, r2;

static void * r(void *obj)
{
    int t1_loop_itr_bnd = 3;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd) {
        int b = load(&gate, memory_order_relaxed);
        if( b == 0 ){
          fetch_add(&(reader_0), 1, memory_order_relaxed);
        }else{
          fetch_add(&(reader_1), 1, memory_order_relaxed);
        }
        // fetch_add(&(readers[b]), 1, memory_order_relaxed);

        int r1 = load(&x, memory_order_relaxed);
        if (r1) {
            int r2 = load(&y, memory_order_relaxed);
            MODEL_ASSERT(!(r1 == 1 && r2 == 2));
        }
        if( b == 0 ) {
          fetch_add(&(reader_0), -1, memory_order_relaxed);
        }else{
          fetch_add(&(reader_1), -1, memory_order_relaxed);
        }
        // fetch_add(&(readers[b]), -1, memory_order_relaxed);
    }

return NULL;}

static void * w(void *obj)
{
    int t2_loop_itr_bnd = 3;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
       store(&x, 0, memory_order_relaxed); // disconnect node

       // wait-for-readers
       int g = load(&gate, memory_order_relaxed);
       if( g == 0 ){
         while (load(&(reader_1), memory_order_relaxed) != 0) thrd_yield();
       }else{
         while (load(&(reader_0), memory_order_relaxed) != 0) thrd_yield();
       }
       // while (load(&(readers[1-g]), memory_order_relaxed) != 0) thrd_yield();

       store(&gate, 1-g, memory_order_relaxed);
       if( g == 0 ) {
         while (load(&(reader_0), memory_order_relaxed) != 0) thrd_yield(); //was acquire
       }else{
         while (load(&(reader_1), memory_order_relaxed) != 0) thrd_yield(); //was acquire
       }
       // while (load(&(readers[g]), memory_order_relaxed) != 0) thrd_yield(); //was acquire

       store(&y, 2, memory_order_relaxed); // free
   }
return NULL;}

int main(int argc, char **argv)
{
	thrd_t t1, t2;

	store(&x, 1, memory_order_seq_cst);
	store(&y, 1, memory_order_seq_cst);
	store(&gate, 0, memory_order_seq_cst);
	store(&(reader_0), 0, memory_order_seq_cst);
	store(&(reader_1), 0, memory_order_seq_cst);
	// store(&(readers[0]), 0, memory_order_seq_cst);
	// store(&(readers[1]), 0, memory_order_seq_cst);

	// printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, &r, NULL);
	thrd_create(&t2, &w, NULL);

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
//#( rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18) ∧ rf(W#y#_l109_c8,R#y#_l72_c22) ) 
//#
//~


//###############################################
//!-M c11 -u 2
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17) ∧ rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17_u0) ∧ rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17_u0) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17_u1) ∧ rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l103_c17_u1) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u3) ∧ rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_0#_l76_c11,R#reader_0#_l97_c17_u4) ∧ rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_1#_l78_c11,R#reader_1#_l105_c17_u2) ∧ rf(W#gate#_l101_c8,R#gate#_l62_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_1#_l78_c11,R#reader_1#_l105_c17_u3) ∧ rf(W#gate#_l101_c8,R#gate#_l62_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(U#reader_1#_l78_c11,R#reader_1#_l105_c17_u4) ∧ rf(W#gate#_l101_c8,R#gate#_l62_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#reader_0#_l120_c2,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22) ) ∨
//#( rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#reader_0#_l120_c2,R#reader_0#_l97_c17_u2) ∧ rf(W#reader_1#_l121_c2,R#reader_1#_l105_c17_u2) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8_u0,R#y#_l72_c22_u0) ) ∨
//#( rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18) ∧ rf(W#y#_l109_c8,R#y#_l72_c22) ) ∨
//#( rf(W#reader_0#_l120_c2,R#reader_0#_l103_c17) ∧ rf(W#x#_l117_c2,R#x#_l70_c18_u0) ∧ rf(W#y#_l109_c8,R#y#_l72_c22_u0) ) 
//#
//~

