// Examples borrowed from http://www.practicalsynthesis.org/PSynSyn.html
//
// This header is modified to remove dependencies from the CDSChecker
//

/**
 * @file impatomic.h
 * @brief Common header for C11/C++11 atomics
 *
 * Note that some features are unavailable, as they require support from a true
 * C11/C++11 compiler.
 */

#include <stdatomic.h>

#if __cplusplus
#ifndef __MEM_OP_MACROS_H__
#define __MEM_OP_MACROS_H__

#define init(var, val) store(var, val, memory_order_seq_cst)
#define load(var, mo)	({\
			atomic_load_explicit(var, mo );\
		})

#define store(var, val, mo)	 ({\
			atomic_store_explicit(var, val, mo );\
			val;\
		});

#define fetch_add(var, val, mo)	({\
			atomic_fetch_add_explicit(var, name, val, mo );\
		});

#define compare_exchange_strong(var1, var2, val, mo1, mo2)	({\
			atomic_compare_exchange_strong_explicit((var1), (name), (var2), (val), (mo1), (mo2) );\
		});

#endif //__MEM_OP_MACROS_H__

		
#endif


