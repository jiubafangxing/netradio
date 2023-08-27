#ifndef __MEDIA_H__
#define __MEDIA_H__
#include <proto.h>
#include <stdlib.h>
struct 	mlib_listentry_st{
	chnid_t  chnid;
	char * desc;
};
int mlib_getchnlist(struct mlib_listentry_st **, int * );
int mlib_freechnlist(struct mlib_listentry_st *);

ssize_t mlib_readchn(chnid_t , void *, size_t);

#endif
