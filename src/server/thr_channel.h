#ifndef __THR_CHANNEL_H_
#define __THR_CHANNEL_H_
#include "medialib.h"
int thr_channel_create(struct mlib_listentry_st *);
int thr_channel_destroy(struct mlib_listentry_st *);
int thr_channel_destroyAll();
#endif
