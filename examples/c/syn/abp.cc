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
#include <stdlib.h>/* //srand, rand */
#include <time.h>       /* time */

#include "librace.h"
#include "mem_op_macros.h"
#include "model-assert.h"

// std::atomic<bool> Msg("Msg"), Ack("Ack");
atomic_bool Msg("Msg"), Ack("Ack");


// bool lAck, lMsg;
// int lRCnt = 0;
// int lSCnt = 0;
atomic_int lRCnt = 0;
atomic_int lSCnt = 0;

/*
 * possible initialization to the two above (lRCnt, lSCnt) = (0,0),(1,0)Err,(0,1) + data independance to prove correctness
 * */


void * p0(void *arg)
{
  bool lSSt = false;
  int t1_loop_itr_bnd = 3;
  int i_t1 = 0;
  while(++i_t1 <= t1_loop_itr_bnd){
    //while(true){
    bool lAck = load(&Ack,  std::memory_order_relaxed);
    if( lAck == lSSt ){
      lSSt = ! lSSt;
      ++lSCnt;
      store(&Msg, lSSt, std::memory_order_relaxed);
      //point to check invariant
      //model_print("\nlRCnt %d; lSCnt %d\n", lRCnt, lSCnt);
      // int copy_lRCnt = load(&lRCnt, std::memory_order_seq_cst);
      // int copy_lSCnt = load(&lSCnt, std::memory_order_seq_cst);
      int copy_lRCnt = lRCnt;
      // int copy_lSCnt = lSCnt;
      MODEL_ASSERT( (copy_lRCnt == lSCnt) || (copy_lRCnt + 1 == lSCnt) );
      // MODEL_ASSERT( (copy_lRCnt == copy_lSCnt) || (copy_lRCnt + 1 == copy_lSCnt) );
    }
  }//}end while true
return NULL;}

void * p1(void *arg)
{
  int t2_loop_itr_bnd = 3;
  int i_t2 = 0;
  bool lRSt = false;
  while(++i_t2 <= t2_loop_itr_bnd){
    //while(true){
    bool lMsg = load(&Msg,  std::memory_order_relaxed);
    if( lMsg != lRSt ){
      lRSt = lMsg;
      ++lRCnt;
      store(&Ack, lRSt, std::memory_order_relaxed);
      //point to check invariant
      //model_print("\nlRCnt %d; lSCnt %d\n", lRCnt, lSCnt);
      int copy_lSCnt = lSCnt;
      MODEL_ASSERT(  (lRCnt == copy_lSCnt) || (lRCnt + 1 == copy_lSCnt)  );
    }
  }//}end while true
  return NULL;}

int main(int argc, char **argv)
{
    thrd_t a, b;
    /* initialize random seed: */
    //srand (time(NULL));
    store(&Msg, false, std::memory_order_seq_cst);
    store(&Ack, false, std::memory_order_seq_cst);

    thrd_create(&a, p0, NULL);
    thrd_create(&b, p1, NULL);

    thrd_join(a);
    thrd_join(b);

    ////model_print("\nlRCnt %d; lSCnt %d\n", lRCnt, lSCnt);
    //MODEL_ASSERT(  (lRCnt == lSCnt) || (lRCnt + 1 == lSCnt)  );
    return 0;
}

//###############################################
//!-M c11 -u 1
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( !hb(U#lSCnt#_l46_c7,R#lSCnt#_l74_c24) ∧ rf(W#Msg#_l47_c7,R#Msg#_l67_c17) ) 
//#
//~


//###############################################
//!-M c11 -u 2
//###############################################
//~
//#
//#Final result
//#Bad DNF
//#( !hb(U#lRCnt#_l70_c7,R#lRCnt#_l52_c24_u0) ∧ rf(W#Ack#_l71_c7,R#Ack#_l43_c17_u0) ) ∨
//#( !hb(U#lRCnt#_l70_c7_u0,R#lRCnt#_l52_c24_u0) ∧ rf(W#Ack#_l71_c7_u0,R#Ack#_l43_c17_u0) ) ∨
//#( !hb(U#lSCnt#_l46_c7,R#lSCnt#_l74_c24) ∧ rf(W#Msg#_l47_c7,R#Msg#_l67_c17) ) ∨
//#( !hb(U#lSCnt#_l46_c7,R#lSCnt#_l74_c24_u0) ∧ rf(W#Msg#_l47_c7,R#Msg#_l67_c17_u0) ∧ rf(W#Msg#_l85_c5,R#Msg#_l67_c17) ) ∨
//#( !hb(U#lSCnt#_l46_c7_u0,R#lSCnt#_l74_c24_u0) ∧ rf(W#Msg#_l47_c7_u0,R#Msg#_l67_c17_u0) ) 
//#
//~


