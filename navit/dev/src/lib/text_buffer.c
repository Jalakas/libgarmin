#include <string.h>
#include <stdlib.h>
#include "text_buffer.h"


#ifdef cplusplus_
extern "C" {
#endif

static int tb_resize(tb_t *b,int add)
{
	char *text;
	
//	printf("resize: size:[%d] offset:[%d]\n",b->size,b->offset);
	if(b->limit && b->limit < b->size )
	{
		return 0;
	}
	text=(char *)realloc(b->text,b->size+add + 200 + 1);
	if(!text)	return 0;
	
	b->text=text;
	b->size+=add+200;
	
	return 1;
}

int tb_insert_text_at(tb_t *buf,char *text,int size,int off)
{
	int rc;
	
	if(buf->size < buf->offset+size)
	{
		rc=tb_resize(buf,size);
		if(!rc) return 0;
	}
	
	if(off>=buf->size)
	{
		rc=tb_resize(buf,(off+size)-buf->size);
		if(!rc) return 0;
	
	}
	memmove(buf->text+size+off,buf->text+off,buf->offset-off);
	memcpy(buf->text+off,text,size);
	buf->offset+=size;
	buf->text[buf->offset]=0;
	return size;
}


int tb_append_text(tb_t *buf,char *text,int size)
{
	int rc;
	
	if( buf->size < buf->offset+size )
	{
		rc=tb_resize(buf,size);
		if(!rc) return 0;
	}
	
	memcpy(buf->text+buf->offset,text,size);
	buf->offset+=size;
	buf->text[buf->offset]=0;
	return size;
}

int tb_prepend_text(tb_t *buf,char *text,int size)
{
/*
	int rc;
	
	if(buf->size < buf->offset+size)
	{
		rc=tb_resize(buf,size);
		if(!rc) return 0;
	}
	memmove(buf->text+size,buf->text,buf->offset);
	memcpy(buf->text,text,size);
	buf->offset+=size;
	buf->text[buf->offset]=0;
	return size;
*/
	return tb_insert_text_at(buf,text,size,0);
}



// prepend ?
// insert at

char *tb_get_text(tb_t *buf)
{
	if(buf->flags&TB_CLEAR_ON_GET)
	{
		buf->offset=0;
	}
	return buf->text;
}

void tb_clear(tb_t *buf)
{
	buf->offset=0;
}

int tb_get_size(tb_t *buf)
{
	return buf->size;
}

int tb_text_size(tb_t *buf)
{
	return buf->offset;
}

void tb_set_limit(tb_t *b,size_t s)
{
	b->limit=s;
}

int tb_init(tb_t *buf,size_t size,int flags)
{
	buf->text=0;
	buf->size=0;
	buf->offset=0;
	buf->flags=flags;
	buf->limit=0;
	if(!tb_resize(buf,size)) return 0;
	return 1;
}

#ifdef cplusplus_
}
#endif

