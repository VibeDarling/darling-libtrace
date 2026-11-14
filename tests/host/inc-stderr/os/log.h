#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct os_log_s *os_log_t;
typedef enum {
	OS_LOG_TYPE_DEFAULT = 0x00,
	OS_LOG_TYPE_INFO = 0x01,
	OS_LOG_TYPE_DEBUG = 0x02,
	OS_LOG_TYPE_ERROR = 0x10,
	OS_LOG_TYPE_FAULT = 0x11,
} os_log_type_t;
void _os_log_impl(void *dso, os_log_t log, os_log_type_t type, const char *format, uint8_t *buf, uint32_t size);
