#include <sys/time.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>
#include "file.h"
#include "debug.h"
#include "log.h"
#include "text_buffer.h"

int debug_level=0;
static GHashTable *debug_hash;

#if 0
static void sigsegv(int sig)
{
	FILE *f;
	time_t t;
	printf("segmentation fault received\n");
	f=fopen("crash.txt","a");
	setvbuf(f, NULL, _IONBF, 0);
	fprintf(f,"segmentation fault received\n");
	t=time(NULL);
	fprintf(f,"Time: %s", ctime(&t));
	file_unmap_all();
	fprintf(f,"dumping core\n");
	fclose(f);	
	abort();
}
#endif



static void
debug_update_level(gpointer key, gpointer value, gpointer user_data)
{
	if (debug_level < (int) value)
		debug_level=(int) value;
}

void
debug_level_set(const char *name, int level)
{
	debug_level=0;
	g_hash_table_insert(debug_hash, g_strdup(name), (gpointer) level);
	g_hash_table_foreach(debug_hash, debug_update_level, NULL);	
}

int
debug_level_get(const char *name)
{
	return (int)(g_hash_table_lookup(debug_hash, name));
}

void
debug_vprintf(int level, const char *module, const int mlen, const char *function, const int flen, int prefix, const char *fmt, va_list ap)
{
	char buffer[mlen+flen+3];

	sprintf(buffer, "%s:%s", module, function);
	if (debug_level_get(module) >= level || debug_level_get(buffer) >= level) {
		if (prefix)
			printf("%s:",buffer);
		vprintf(fmt, ap);
	}
}

void
debug_printf(int level, const char *module, const int mlen,const char *function, const int flen, int prefix, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	debug_vprintf(level, module, mlen, function, flen, prefix, fmt, ap);
	va_end(ap);
}


struct log_file {
	FILE *logfp;
	int flags;
	log_prefix_fn prefix_fn;
	char *filename;
	ssize_t maxsize;
	int flush_size;
	int flush_time;
	tb_t tbuf;
};

static int logfile_open(struct log_file *lf)
{
	char *mode = "a";
	if (lf->flags&LF_OVERWRITE)
		mode = "w";
	if (lf->filename)
		lf->logfp = fopen(lf->filename, mode);
	else {
		lf->logfp = stderr;
	}
	if (lf->logfp)
		return 1;
	return -1;
}

static void logfile_close(struct log_file *lf)
{
	if (lf->logfp)
		fclose(lf->logfp);
}

void logfile_free(struct log_file *lf)
{
	if (lf->logfp)
		fclose(lf->logfp);
	if (lf->filename)
		free(lf->filename);
	free(lf);

}
struct log_file *logfile_alloc(int flags, char *file, log_prefix_fn prefix_fn,
		int maxsize, int flush_size, int flush_time)
{
	struct log_file *lf;
	lf = calloc(1, sizeof(*lf));
	lf->flags = flags;
	if (file) {
		lf->filename = strdup(file);
		if (!lf->filename) {
			free(lf);
			return NULL;
		}
	} else {
		lf->flags &= ~LF_REOPEN;
	}
	lf->prefix_fn = prefix_fn;
	lf->maxsize = maxsize;
	lf->flush_size = flush_size;
	lf->flush_time = flush_time;
	if (flags & LF_BUFFERED) {
		tb_init(&lf->tbuf, flush_size ? : 4096, TB_CLEAR_ON_GET);
	}

	if (!(lf->flags & LF_REOPEN))
		if (logfile_open(lf))
			return lf;
	if (lf->filename)
		free(lf->filename);
	free(lf);

	return NULL;
}

void logfile_log(struct log_file *lf, int l, const char *file, const int line, const char *function, const char *fmt, va_list ap)
{
//	va_list ap;

	if (lf->flags&LF_REOPEN) {
		if (logfile_open(lf) < 0) {
			fprintf(stderr, "Can not open logfile:[%s]\n",
				lf->filename);
			return;
		}
	}
	if (lf->prefix_fn)
		lf->prefix_fn(lf->logfp, l, file, line, function);
//	va_start(ap, fmt);
	vfprintf(lf->logfp, fmt, ap);
//	va_end(ap);
	if (lf->flags&LF_REOPEN) {
		logfile_close(lf);
	} else 
		fflush(lf->logfp);
}


int navit_debug_level = 10;
static struct log_file *debuglf;

static int prefix_debug(FILE *fp, const int l, const char *file, const int line, const char *function)
{
	struct timeval tv;
	struct tm tm;
	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);
	fprintf(fp, "%d:%d:%d.%d|%d|%s:%d|%s|",
		tm.tm_hour, tm.tm_min, tm.tm_sec, (int)tv.tv_usec,
		l, file, line, function);
	return 1;
}

void 
debug_log(const int level, const char *file, const int line, const char *function, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	logfile_log(debuglf, level, file, line, function, fmt, ap);
	va_end(ap);
}


void
debug_init(void)
{
#if 0
	signal(SIGSEGV, sigsegv);
#endif
	debug_hash=g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
	debuglf = logfile_alloc(LF_OVERWRITE, "navit.log", prefix_debug, 0, 0, 0);
}
