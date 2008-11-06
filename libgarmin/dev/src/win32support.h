#ifndef _WIN32_SUPPORT_H_
#include <windows.h>
#include <winbase.h>
#include <winerror.h>
typedef unsigned char u_int8_t;
typedef unsigned short int u_int16_t;
typedef unsigned int u_int32_t;
#define EPERM ERROR_ACCESS_DENIED

#define sync() do {} while(0)
#define errno GetLastError()
#define strerror(x) ""
#define _WIN32_SUPPORT_H_
#endif
