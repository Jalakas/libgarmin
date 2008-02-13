#ifndef __TEXT_BUFFER_H_
#define __TEXT_BUFFER_H_
#define tb_t struct text_buffer

#define TB_CLEAR_ON_GET 0x00000001
// #define TB_CLEAR_ON_GET 0x00000001

#ifdef cplusplus_
extern "C" {
#endif

tb_t
{
	char *text;
	int size;
	int offset;
	int flags;
	int limit;
};

int tb_append_text(tb_t *buf,char *text,int size);
int tb_prepend_text(tb_t *buf,char *text,int size);
char *tb_get_text(tb_t *buf);
int tb_init(tb_t *buf,size_t size,int flags);
int tb_get_size(tb_t *b);
int tb_text_size(tb_t *buf);
void tb_clear(tb_t *t);
void tb_set_limit(tb_t *b,size_t s);
#ifdef cplusplus_
}
#endif

#endif

