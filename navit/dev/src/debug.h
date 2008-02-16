#ifndef NAVIT_DEBUG_H
#define NAVIT_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
extern int debug_level;
#define dbg_str2(x) #x
#define dbg_str1(x) dbg_str2(x)
#define dbg_module dbg_str1(MODULE)
#define dbg(level,fmt...) ({ if (debug_level >= level) debug_printf(level,dbg_module,strlen(dbg_module),__PRETTY_FUNCTION__, strlen(__PRETTY_FUNCTION__),1,fmt); })

/* prototypes */
void debug_init(void);
void debug_level_set(const char *name, int level);
int debug_level_get(const char *name);
void debug_vprintf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap);
void debug_printf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, ...);
/* end of prototypes */

#define LF_OVERWRITE	(1<<0)
#define LF_ROTATE	(1<<1)
#define LF_BUFFERED	(1<<2)
#define LF_REOPEN	(1<<3)
struct log_file;
typedef int (*log_prefix_fn)(FILE *fp, const int level, const char *file, const int line, const char *function);
struct log_file *logfile_alloc(int flags, char *file, log_prefix_fn prefix_fn, int maxsize, int flush_size, int flush_time);
void logfile_free(struct log_file *lf);
void logfile_log(struct log_file *lf, int l, const char *file, const int line, const char *function, const char *fmt, va_list ap);

void 
debug_log(const int level, const char *file, const int line, const char *function, const char *fmt, ...);
extern int navit_debug_level;

#define debug(l,f...)								\
	do {									\
		if (l<=navit_debug_level)					\
			debug_log(l, __FILE__, __LINE__, __FUNCTION__, ## f);	\
	} while(0)

#define ENTER()		debug(10, "ENTER\n")
#ifdef __cplusplus
}
#endif

#endif

