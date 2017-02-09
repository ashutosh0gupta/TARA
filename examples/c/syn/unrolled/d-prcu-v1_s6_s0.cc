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
#include <stdio.h>
#include <threads.h>
#include <stdatomic.h>
#include <model-assert.h>

#include "librace.h"

#include "mem_op_macros.h"

atomic<int> readers("readers");
atomic<int> x("x");
atomic<int> y("y");

int r1, r2;

static void r(void *obj)
{
    int t1_loop_itr_bnd = 1;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){
        fetch_add(&readers, 1, memory_order_seq_cst);

        r1 = load(&x, memory_order_seq_cst);
        if (r1){
            r2 = load(&y, memory_order_seq_cst);
            MODEL_ASSERT(!(r1 == 1 && r2 == 2));
        }

        fetch_add(&readers, -1, memory_order_relaxed);
    }
}

static void w(void *obj)
{
    int t2_loop_itr_bnd = 1;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
        store(&x, 0, memory_order_seq_cst); // disconnect node
        
        // wait-for-readers
        while (load(&readers, memory_order_seq_cst) != 0){
            thrd_yield();
        }

        store(&y, 2, memory_order_seq_cst); // free
    }
}

int user_main(int argc, char **argv)
{
    thrd_t t1, t2;

    store(&x, 1,std::memory_order_seq_cst);
    store(&y, 1,std::memory_order_seq_cst);
    store(&readers, 0,std::memory_order_seq_cst);

    printf("Main thread: creating 2 threads\n");
    thrd_create(&t1, (thrd_start_t)&r, NULL);
    thrd_create(&t2, (thrd_start_t)&w, NULL);

    thrd_join(t1);
    thrd_join(t2);

    MODEL_ASSERT(!(r1 == 1 && r2 == 2));

    printf("Main thread is finished\n");

    return 0;
}
