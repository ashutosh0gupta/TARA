#ifndef __MODEL_ASSERT_H__
#define __MODEL_ASSERT_H__

#if __cplusplus
extern "C" {
#else
// #include <stdbool.h>
#endif

void assume( bool );
void assert( bool );

#define MODEL_ASSERT(expr) assert( expr )

#if __cplusplus
}
#endif

#endif /* __MODEL_ASSERT_H__ */
