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

#define PUBSUB_MALLOC malloc
#define PUBSUB_CALLOC calloc
#define PUBSUB_FREE free
#define PUBSUB_ASSERT(x) assert(x)

#elseif defined(PLATFORM_FREERTOS)

#define PUBSUB_MALLOC malloc
#define PUBSUB_CALLOC calloc
#define PUBSUB_FREE free
#define PUBSUB_ASSERT(x) assert(x)

#elseif defined(PLATFORM_RTTHREAD)

#define PUBSUB_MALLOC rt_malloc
#define PUBSUB_CALLOC rt_calloc
#define PUBSUB_FREE rt_free
#define PUBSUB_ASSERT(x) rt_assert(x)

#else

#error "Please define a valid platform: PLATFORM_LINUX, PLATFORM_FREERTOS, PLATFORM_RTTHREAD"

#endif

#ifdef __cplusplus
}
#endif

#endif
