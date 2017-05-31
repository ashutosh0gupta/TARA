/**
 * @file arm.h
 * @brief Common header for ARM8 like read/writes for TARA
 *
 * chosen mapping to c11 atomics
 *   L -> rlease
 *   A -> acquire
 *   Q -> seq_cst
 *   R -> relaxed
 *   W -> relaxed
 *
 *  TARA is aware of the above mapping and generates constraints accordingly.
 *
 *  The above mapping in not standard. It is chosen for convinence.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>

#if __cplusplus
#ifndef __MEM_OP_MACROS_H__
#define __MEM_OP_MACROS_H__

// #define init(var, val) store(var, val, memory_order_seq_cst)

#define loadA(var) ({atomic_load_explicit(var, memory_order_acquire );})
#define loadQ(var) ({atomic_load_explicit(var, memory_order_seq_cst );})
#define load(var)  ({atomic_load_explicit(var, memory_order_relaxed );})

#define storeL(x,v) ({atomic_store_explicit(x,v,memory_order_release);v;});
#define store(x,v) ({atomic_store_explicit(x,v,memory_order_relaxed);v;});

#define fa_int(var, val, mo) ({atomic_fetch_add_explicit(var, val, mo );});

#define fetch_addLA(var, val) fa_int( var, val, memory_order_acq_rel)
#define fetch_addA(var, val)  fa_int( var, val, memory_order_acquire)
#define fetch_addL(var, val)  fa_int( var, val, memory_order_release)
#define fetch_add(var, val)   fa_int( var, val, memory_order_relaxed)


#define xchng(var, val, mo) ({atomic_exchange_explicit(var, val, mo );});

#define exchangeLA(var, val) xchng( var, val, memory_order_acq_rel)
#define exchangeA(var, val)  xchng( var, val, memory_order_acquire)
#define exchangeL(var, val)  xchng( var, val, memory_order_release)
#define exchange(var, val)   xchng( var, val, memory_order_relaxed)


#define cas_i(x, v1, v2, mo1, mo2) ({\
      atomic_compare_exchange_strong_explicit(x, v1, v2, mo1, mo2); \
    });

#define casLAA(x,v1,v2) cas_i(x,v1,v2,memory_order_acq_rel,memory_order_acquire)
#define casLA(x,v1,v2)  cas_i(x,v1,v2,memory_order_acq_rel,memory_order_relaxed)
#define casL(x,v1,v2)   cas_i(x,v1,v2,memory_order_release,memory_order_relaxed)
#define casA(x,v1,v2)   cas_i(x,v1,v2,memory_order_acquire,memory_order_relaxed)
#define casAA(x,v1,v2)  cas_i(x,v1,v2,memory_order_acquire,memory_order_acquire)
#define cas(x,v1,v2)    cas_i(x,v1,v2,memory_order_relaxed,memory_order_relaxed)

void fence(); // for dmb_full
void fence_arm8_dmb_ld();
void fence_arm8_dmb_st();

void assume( bool );
void assert( bool );

#endif //__MEM_OP_MACROS_H__
#endif


