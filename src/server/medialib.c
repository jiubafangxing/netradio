#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glob.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include "medialib.h"
#include "mytbf.h"
#include <proto.h>
#include "server.h"
#define PATHSIZE 1024
#define LINEBUFSIZE 1024
#define MP3_BITRATE 1024
struct  channel_context_st {
	chnid_t chnid;
	char * desc;
	glob_t mp3glob;
	//被读取的mp3文件在mp3glob中的位置
	int pos;
	int fd;
	off_t offset;
	mytbf_t *tbf;

};
static struct channel_context_st channel[MAXCHNID+1];
static struct channel_context_st * analysys(char * dir){
	char pathdir[PATHSIZE];
	char linebuf[LINEBUFSIZE];
	FILE * fp;
	struct channel_context_st *me;
	static chnid_t curr_id = MINCHNID;
	//snprintf(pathdir, PATHSIZE, "%s/*.mp3", dir);
	strncpy(pathdir, dir,PATHSIZE);
	strncat(pathdir, "/desc.txt",PATHSIZE);
	fp =  fopen(pathdir, "r+");
	if(NULL == fp ){
		syslog(LOG_WARNING,"there is'n any desc in %s",dir);
		return NULL;
	}
	if(fgets(linebuf, LINEBUFSIZE,fp) == NULL){
		syslog(LOG_WARNING,"can not read desc  in %s",dir);
		return NULL;
	}
	fclose(fp);
	me = malloc(sizeof(*me));
	me->chnid = curr_id;
	me->desc = strdup(linebuf);
	me->tbf = mytbf_init(MP3_BITRATE/8 *15,MP3_BITRATE/8*20);
	if(me->tbf == NULL){
		syslog(LOG_WARNING,"can not read desc  in %s",dir);
		return NULL;
	}
	strncpy(pathdir, dir,PATHSIZE);
	strncat(pathdir, "/*.mp3",PATHSIZE);
	if(glob(pathdir, 0,NULL, &me->mp3glob) < 0 ){
		syslog(LOG_WARNING,"can not find mp3 files desc  in %s",dir);
		fclose(fp);
		curr_id ++;
		return NULL;
	}
	me->offset = 0;
	me->pos = 0;
	curr_id+=1;
	int fd = 	open(me->mp3glob.gl_pathv[me->pos],O_RDONLY);
	if(fd < 0){
		syslog(LOG_ERR,"open mp3 file %s  error", me->mp3glob.gl_pathv[me->pos]	);
		free(me);
	}
	me->fd = fd;
	return me;
}

int mlib_getchnlist(struct mlib_listentry_st ** mls, int * resultsize){
	char  dir[PATHSIZE];	
	glob_t  globt;
	struct mlib_listentry_st * ptr;	
	struct channel_context_st * channelContextSt;	
	snprintf(dir,PATHSIZE,"%s/*",serverConf.media_dir);
	int num = 0;
	for(int i = 0; i< MAXCHNID +1 ; i++){
		channel[i].chnid = -1;
	}
	int globResult = glob(dir,GLOB_ONLYDIR,NULL,&globt);
	ptr = malloc(sizeof(struct mlib_listentry_st) * globt.gl_pathc);
	if(globResult < 0){
		syslog(LOG_ERR, "read mediadir err %s",serverConf.media_dir);
		return -1;
	}else{
		for(int i = 0; i< globt.gl_pathc; i++){
			channelContextSt=  analysys(globt.gl_pathv[i] );
			if(NULL != channelContextSt){
				syslog(LOG_DEBUG,"analysys return %d: %s.",channelContextSt->chnid, channelContextSt->desc);
				memcpy(channel+channelContextSt->chnid,channelContextSt,sizeof(*channelContextSt));
				ptr[num].chnid = channelContextSt->chnid;
				ptr[num].desc = channelContextSt->desc;
				syslog(LOG_INFO, "analysys dir %s success", globt.gl_pathv[i]);	
			}else{
				syslog(LOG_ERR, "analysys dir %s fail", globt.gl_pathv[i]);	
			}
			num++;
		}
		*mls = realloc(ptr,sizeof(struct mlib_listentry_st *)* num);
		*resultsize = num;
	}
	return 0;
}
int mlib_freechnlist(struct mlib_listentry_st * ls){
	free(ls);	
	return 1;
}
int opennext(chnid_t chnid){
	for(int i =0; i< channel[chnid].mp3glob.gl_pathc; i++){
		channel[chnid].pos++;
		if(channel[chnid].pos == channel[chnid].mp3glob.gl_pathc ){
			channel[chnid].pos = 0;
			break;
		}
		close(channel[chnid].fd);
		int fd = open(channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],O_RDONLY);
		if(fd < 0){
			syslog(LOG_ERR, "open file fail%s",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
		}else{
			channel[chnid].offset = 0;
			return 0;
		}
	}
	syslog(LOG_ERR,"open channel[%d] files fail", chnid);
	return -1;
}

ssize_t mlib_readchn(chnid_t chnid, void * buf , size_t size){
	int fetchSize = mytbf_fetchtoken(channel[chnid].tbf,size);
	int len = 0;
	while(1){

		len= 	pread(channel[chnid].fd,buf,fetchSize ,channel[chnid].offset);
		if(len< 0){
			syslog(LOG_ERR,"media file  %s pread(): %s",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos],"err");

			opennext(chnid);
		}else if (len == 0){
			
			syslog(LOG_ERR,"media file  %s pread(): is over",channel[chnid].mp3glob.gl_pathv[channel[chnid].pos]);
			opennext(chnid);
		}else{
			
	syslog(LOG_ERR,"send mp3 content success %d", chnid);
			channel[chnid].offset+= len;
			break;
		}
	}
	if(fetchSize - len> 0){
		mytbf_returntoken(channel[chnid].tbf,fetchSize - len);	
	}
	return len;
}


