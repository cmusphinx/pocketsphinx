/**
 * Define basic types in case there is no <stdint.h>
 */

#ifndef __RTC_BASE_TYPEDEFS_H__
#define __RTC_BASE_TYPEDEFS_H__

#include <pocketsphinx/sphinx_config.h>
#include <pocketsphinx/prim_type.h>

#ifndef HAVE_STDINT_H
typedef int32		int32_t;
typedef int16		int16_t;
typedef int8		int8_t;
typedef uint32		uint32_t;
typedef uint16		uint16_t;
typedef uint8		uint8_t;
typedef int64		int64_t;
typedef uint64		uint64_t;

#define INT32_MAX	MAX_INT32
#define INT32_MIN	MIN_INT32
#endif

#endif /*  __RTC_BASE_TYPEDEFS_H__ */
