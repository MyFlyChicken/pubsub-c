/**
 * @file ps_config.h
 * @author  ()
 * @brief
 * @version 0.1
 * @date 2025-11-07
 *
 * @copyright Copyright (c) 2025
 *
 * @par 修改日志:
 * <table>
 * <caption id="multi_row">$</caption>
 * <tr><th>Date       <th>Version <th>Author  <th>Description
 * <tr><td>2025-11-07 <td>v1.0     <td>yyf     <td>内容
 * </table>
 * @section
 * @code
 * @endcode
 */
#ifndef __ps_config_H_ // shift+U转换为大写
#define __ps_config_H_

#ifdef __cplusplus
extern "C" {
#endif

#if defined(PLATFORM_LINUX)
#include <assert.h>
#include <string.h>

#define PUBSUB_MALLOC malloc
#define PUBSUB_CALLOC calloc
#define PUBSUB_FREE free
#define PUBSUB_ASSERT(x) assert(x)

#elif defined(PLATFORM_FREERTOS)

#define PUBSUB_MALLOC malloc
#define PUBSUB_CALLOC calloc
#define PUBSUB_FREE free
#define PUBSUB_ASSERT(x) assert(x)

#elif defined(PLATFORM_RTTHREAD)

#define PUBSUB_MALLOC rt_malloc
#define PUBSUB_CALLOC rt_calloc
#define PUBSUB_FREE rt_free
#define PUBSUB_ASSERT(x) rt_assert(x)

#elif defined(PLATFORM_ZEPHYR) || defined(__ZEPHYR__) || defined(CONFIG_PUBSUB_C_ZEPHYR)
#include <zephyr/kernel.h>

#define PUBSUB_MALLOC k_malloc
#define PUBSUB_CALLOC k_calloc
#define PUBSUB_FREE k_free
// 使用标准 assert，避免依赖私有断言宏
#define PUBSUB_ASSERT(x) assert(x)

#ifndef strdup
// Zephyr 最小 libc 可能没有 strdup，这里提供一个兼容版本
static inline char *ps_zephyr_strdup(const char *s) {
	size_t n = strlen(s) + 1U;
	char *p = k_malloc(n);
	if (p) {
		memcpy(p, s, n);
	}
	return p;
}
#define strdup ps_zephyr_strdup
#endif // strdup
#else

#include <assert.h>
#include <string.h>

#define PUBSUB_MALLOC malloc
#define PUBSUB_CALLOC calloc
#define PUBSUB_FREE free
#define PUBSUB_ASSERT(x) assert(x)

// #warning
// "Please define a valid platform: PLATFORM_LINUX, PLATFORM_FREERTOS, PLATFORM_RTTHREAD, PLATFORM_ZEPHYR,Now use
// default malloc/free."

#endif // PLATFORM_LINUX

#ifdef __cplusplus
}
#endif

#endif
