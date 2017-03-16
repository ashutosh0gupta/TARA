// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html

/*
 * Dekker's critical section algorithm, implemented with fences.
 *
 * URL:
 *   http://www.justsoftwaresolutions.co.uk/threading/
 *
 */

#include <atomic>
#include "threads.h"
//#include <assert.h>

#include "librace.h"
#include "mem_op_macros.h"

std::atomic<int> Y1("Y1"), Y2("Y2"), Ch1("Ch1"), Ch2("Ch2");

uint32_t critical_section = 0;
/*
 * possible initialization to the two above (lRCnt, lSCnt) = (0,0),(1,0)Err,(0,1) + data independance to prove correctness
 * */


void * p1(void *arg)
{

    int t1_loop_itr_bnd = 2; // 1;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){    //while(true){
        store(&Ch1, 1, std::memory_order_relaxed);
        int t12 = load(&Y2,  std::memory_order_relaxed);
        t12 = t12 + 1;
        store(&Y1, t12,  std::memory_order_relaxed)
        store(&Ch1, 0, std::memory_order_relaxed);

        while(1 == load(&Ch2, std::memory_order_relaxed)) thrd_yield();

        int t14;

        while(0 < (t14 = load(&Y2,  std::memory_order_relaxed)) && t12 >= t14) thrd_yield();

        store_32(&critical_section, 1);

        store(&Y1, 0,  std::memory_order_relaxed)

    }//}end while true
return NULL;}

void * p2(void *arg)
{
    int t2_loop_itr_bnd = 2; // 1;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
        //while(true){
        store(&Ch2, 1, std::memory_order_relaxed);
        int t21 = load(&Y1,  std::memory_order_relaxed);
        t21 = t21 + 1;
        store(&Y2, t21,  std::memory_order_relaxed)
        store(&Ch2, 0, std::memory_order_relaxed);

        while(1 == load(&Ch1, std::memory_order_relaxed))     thrd_yield();

        int t24;

        while(0 < (t24 = load(&Y1,  std::memory_order_relaxed)) && t21 > t24) thrd_yield();

        store_32(&critical_section, 1);

        store(&Y2, 0,  std::memory_order_relaxed)
    }//}end while true
return NULL;}

int main(int argc, char **argv)
{
    thrd_t a, b;

    store(&Y1, 0, std::memory_order_seq_cst);
    store(&Y2, 0, std::memory_order_seq_cst);
    store(&Ch1, 0, std::memory_order_seq_cst);
    store(&Ch2, 0, std::memory_order_seq_cst);

    thrd_create(&a, p1, NULL);
    thrd_create(&b, p2, NULL);

    thrd_join(a);
    thrd_join(b);

    return 0;
}
