#include "config.h"

#ifdef TARGET_WIN32CE
static inline unsigned int
get_u32(void *ptr) {
	unsigned char *p = ptr;
	unsigned long ret;
	ret=*p++;
	ret|=(*p++) << 8;
	ret|=(*p++) << 16;
	ret|=(*p++) << 24;
	return ret;
}

static inline unsigned int
get_u24(void *ptr) {
	unsigned char *p = ptr;
	unsigned int ret;
	ret=*p++;
	ret|=(*p++) << 8;
	ret|=(*p++) << 16;
	return ret;
}

static inline unsigned short
get_u16(void *ptr) {
	unsigned char *p = ptr;
	unsigned short ret;
	ret=*p++;
	ret|=(*p++) << 8;
	return ret;
}

static inline unsigned char
get_u8(void *ptr)
{
	unsigned char *p = ptr;
	return *p;
}
#else
#define get_u32(ptr) (*(u_int32_t *)ptr)
#define get_u24(ptr) ((*(u_int32_t*)ptr) & 0x00FFFFFF)
#define get_u16(ptr) (*(u_int16_t *)ptr)
#define get_u8(ptr)  (*(u_int8_t *)ptr)
#endif
