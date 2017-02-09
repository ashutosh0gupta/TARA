// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html

/*
 * Dekker's critical section algorithm, implemented with fences.
 *
 * URL:
 *   http://www.justsoftwaresolutions.co.uk/threading/
 */

#include <atomic>
#include <threads.h>

#include "librace.h"
#include "mem_op_macros.h"

std::atomic<bool> flag0("flag0"), flag1("flag1");
std::atomic<int> turn("turn");

uint32_t critical_section = 0;

void p0(void *arg)
{

    int t1_loop_itr_bnd = 1;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){
        store(&flag0, true,std::memory_order_relaxed);

        while (load(&flag1, std::memory_order_relaxed))
        {
            if (load(&turn, std::memory_order_relaxed) != 0)
            {
                store(&flag0, false,std::memory_order_relaxed);
                while (load(&turn, std::memory_order_relaxed) != 0)
                {
                    thrd_yield();
                }
                store(&flag0, true,std::memory_order_relaxed);
                //std::atomic_thread_fence seq_cst;
            } else
                thrd_yield();
        }
        // critical section
        store_32(&critical_section, 1);

        store(&turn, 1,std::memory_order_relaxed);
        store(&flag0, false,std::memory_order_relaxed);
    }
}

void p1(void *arg)
{

    int t2_loop_itr_bnd = 1;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
        store(&flag1, true,std::memory_order_relaxed);

        while (load(&flag0, std::memory_order_relaxed))
        {
            if (load(&turn, std::memory_order_relaxed) != 1)
            {
                store(&flag1, false,std::memory_order_relaxed);
                while (load(&turn, std::memory_order_relaxed) != 1)
                {
                    thrd_yield();
                }
                store(&flag1, true,std::memory_order_relaxed);
            } else
                thrd_yield();
        }

        // critical section
        store_32(&critical_section, 2);

        store(&turn, 0,std::memory_order_relaxed);
        //std::atomic_thread_fence release);
        store(&flag1, false,std::memory_order_relaxed);
    }
}

int user_main(int argc, char **argv)
{
    thrd_t a, b;

    store(&flag0, false,std::memory_order_seq_cst);
    store(&flag1, false,std::memory_order_seq_cst);
    store(&turn, 0,std::memory_order_seq_cst);

    thrd_create(&a, p0, NULL);
    thrd_create(&b, p1, NULL);

    thrd_join(a);
    thrd_join(b);

    return 0;
}
