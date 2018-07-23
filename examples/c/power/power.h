/**
 * @file power.h
 * @brief Common header for POWER like fences and read/writes for TARA
 *
 */

#include <stdio.h>
#include <stdlib.h>
// #include <stdatomic.h>
#include <pthread.h>

#if __cplusplus
#ifndef __MEM_OP_MACROS_H__
#define __MEM_OP_MACROS_H__

void power_full_sync();
void power_lwsync();
void power_isync();

void assume( bool );
void assert( bool );

#endif //__MEM_OP_MACROS_H__
#endif


