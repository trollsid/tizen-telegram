/*
 * logger.h
 *
 *  Created on: Jun 25, 2015
 *      Author: sandeep
 */

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif
#define LOG_TAG "hyb_service"

#define MODULE_INFO (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define INF(fmt, arg...) dlog_print(DLOG_INFO, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define LOGI(fmt, arg...) dlog_print(DLOG_INFO, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define DBG(fmt, arg...) dlog_print(DLOG_DEBUG, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define LOGD(fmt, arg...) dlog_print(DLOG_DEBUG, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define WARN(fmt, arg...) dlog_print(DLOG_WARN, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define ERR(fmt, arg...) dlog_print(DLOG_ERROR, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define LOGE(fmt, arg...) dlog_print(DLOG_ERROR, \
        LOG_TAG, "%s: %s(%d) > " fmt, MODULE_INFO, \
        __func__, __LINE__, ##arg)

#define RETM_IF(expr, fmt, arg...) \
{ \
    if (expr) { \
        ERR(fmt, ##arg); \
        return; \
    } \
}

#define RETVM_IF(expr, val, fmt, arg...) \
{ \
    if (expr) { \
        ERR(fmt, ##arg); \
        return (val); \
    } \
}

#define retv_if(expr, val) do { \
	if (expr) { \
		ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define ret_if(expr) do { \
	if (expr) { \
		ERR("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define goto_if(expr, val) do { \
	if (expr) { \
		ERR("(%s) -> goto", #expr); \
		goto val; \
	} \
} while (0)

#define break_if(expr) { \
	if (expr) { \
		ERR("(%s) -> break", #expr); \
		break; \
	} \
}

#define continue_if(expr) { \
	if (expr) { \
		ERR("(%s) -> continue", #expr); \
		continue; \
	} \
}



#endif /* __LOGGER_H__ */
