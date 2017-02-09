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
#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
#include <model-assert.h>

#include "librace.h"

#include "common.h"

#include "mem_op_macros.h"

atomic<int> gate("gate");
atomic<int> readers[2] = {atomic<int>("readers1"),atomic<int>("readers2")};
atomic<int> x("x");
atomic<int> y("y");

int r1, r2;

static void r(void *obj)
{
    int t1_loop_itr_bnd = 2; // 1;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){
        int b = load(&gate, memory_order_acquire);
        fetch_add(&(readers[b]), 1, memory_order_seq_cst);

        r1 = load(&x, memory_order_seq_cst);
        if (r1)
            r2 = load(&y, memory_order_relaxed);

        fetch_add(&(readers[b]), -1, memory_order_seq_cst);
    }

}

static void w(void *obj)
{
    int t2_loop_itr_bnd = 2; // 1;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
       store(&x, 0, memory_order_seq_cst); // disconnect node

       // wait-for-readers
       int g = load(&gate, memory_order_relaxed);
       while (load(&(readers[1-g]), memory_order_relaxed) != 0) thrd_yield();
       store(&gate, 1-g, memory_order_release);
       while (load(&(readers[g]), memory_order_seq_cst) != 0) thrd_yield(); //was acquire

       store(&y, 2, memory_order_relaxed); // free
   }
}

int user_main(int argc, char **argv)
{
	thrd_t t1, t2;

	store(&x, 1, memory_order_seq_cst);
	store(&y, 1, memory_order_seq_cst);
	store(&gate, 0, memory_order_seq_cst);
	store(&(readers[0]), 0, memory_order_seq_cst);
	store(&(readers[1]), 0, memory_order_seq_cst);

	printf("Main thread: creating 2 threads\n");
	thrd_create(&t1, (thrd_start_t)&r, NULL);
	thrd_create(&t2, (thrd_start_t)&w, NULL);

	thrd_join(t1);
	thrd_join(t2);

	MODEL_ASSERT(!(r1 == 1 && r2 == 2));

	printf("Main thread is finished\n");

	return 0;
}
