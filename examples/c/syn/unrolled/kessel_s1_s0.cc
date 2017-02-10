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
#include <assert.h>

#include "librace.h"
#include "mem_op_macros.h"

std::atomic<bool> Q0("Q0"), Q1("Q1");
std::atomic<int> R0("R0"), R1("R1");

uint32_t critical_section = 0;
/*
 * possible initialization to the two above (lRCnt, lSCnt) = (0,0),(1,0)Err,(0,1) + data independance to prove correctness
 * */


void * p0(void *arg)
{
    int t1_loop_itr_bnd = 1;
    int i_t1 = 0;
    while(++i_t1 <= t1_loop_itr_bnd){
		//while(true){
		store(&Q0, true, std::memory_order_relaxed);
		int local = load(&R1,  std::memory_order_relaxed);
		store(&R0, local,  std::memory_order_relaxed);
		int loop_bnd = 0;
		while(load(&Q1, std::memory_order_relaxed) && local == load(&R1, std::memory_order_relaxed) &&
			++loop_bnd <= 2){
			thrd_yield();
		}
		if(loop_bnd > 2) continue;

		store_32(&critical_section, 1); //  critical section

		store(&Q0, false,  std::memory_order_relaxed)

	//}end while true
	}
}

void * p1(void *arg)
{
    int t2_loop_itr_bnd = 1;
    int i_t2 = 0;
    while(++i_t2 <= t2_loop_itr_bnd){
	//while(true){
		store(&Q1, true, std::memory_order_relaxed);
		int local = 1 - load(&R0,  std::memory_order_relaxed);
		store(&R1, local, std::memory_order_relaxed);
		int loop_bnd = 0;
		while( load(&Q0, std::memory_order_relaxed) && local == 1 - load(&R0, std::memory_order_relaxed) 
			&& (++loop_bnd <= 2)){ 
			thrd_yield(); 
		}
		if(loop_bnd > 2) continue;
		
		store_32(&critical_section, 1); //  critical section
		

		store(&Q1, false,  std::memory_order_relaxed)
	}
	//}end while true
}

int main(int argc, char **argv)
{
	thrd_t a, b;

	store(&Q0, false, std::memory_order_seq_cst);
	store(&Q1, false, std::memory_order_seq_cst);
	store(&R0, 0, std::memory_order_seq_cst);
	store(&R1, 0, std::memory_order_seq_cst);

	thrd_create(&a, p0, NULL);
	thrd_create(&b, p1, NULL);

	thrd_join(a);
	thrd_join(b);

	return 0;
}
