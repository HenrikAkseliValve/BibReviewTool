/*
*  Small support functions which don't need a module.
*/
#ifndef _STDLIB_EXT_H_
#define _STDLIB_EXT_H_
#include<stdint.h>

#if defined(__cplusplus__) || defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/*
* Functions for writing 32bit integer to a string buffer.
*/
uint32_t i32toalen(int32_t integer);
void i32toa(const int32_t integer,uint8_t *buffer,const uint32_t len);
/*
* Function for writing 64bit integer to string buffer.
*/
uint32_t i64toalen(int64_t integer);
void i64toa(const int64_t integer,uint8_t *buffer,const uint32_t len);

#if defined(__cplusplus__) || defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif /*_STDLIB_EXT_H_*/
