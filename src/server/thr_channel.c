#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include "thr_channel.h"
#include "medialib.h"
#include "server_conf.h"
#include <proto.h>
struct thr_channel_ent_sdt{
	chnid_t chnid;
	pthread_t tid;

};

struct thr_channel_ent_sdt thr_channel[CHNNR];
int tid_nextpos = 0;
static void * thr_channel_snder(void * pptr){
	struct mlib_listentry_st * ptr= pptr;
	while(1){
		struct msg_channel_st * buf ;
		buf = malloc(MSG_CHANNEL_MAX);
		buf->chnid = ptr->chnid;
		ssize_t len = mlib_readchn(buf->chnid,buf->data,MAX_DATA);
		struct sockaddr_in addr = sndaddr;
		int sendResult = sendto(serversd,buf,sizeof(chnid_t)+len,0,(void *)&sndaddr,sizeof(addr));
		if(sendResult < 0){
			syslog(LOG_ERR, "fail to send data to {%d}", ptr->chnid);
		}else{
		
			syslog(LOG_INFO, "success to send data to {%d}", ptr->chnid);
		}
		sched_yield();
	}
	pthread_exit(0);
}
int thr_channel_create(struct mlib_listentry_st * ptr ){
	int result = pthread_create(&thr_channel[tid_nextpos].tid,NULL,thr_channel_snder,(void *)ptr);
	if(result < 0){
		syslog(LOG_ERR,"create thread fail for %d", ptr->chnid);
		return -1;
	}
	thr_channel[tid_nextpos].chnid = ptr->chnid;
	tid_nextpos +=1;
	return 0;
}
int thr_channel_destroy(struct mlib_listentry_st * ptr){
	for(int i = 0; i<  CHNNR; i++){
		if(thr_channel[i].chnid == ptr->chnid ){
			if(thr_channel[i].tid < 0){
				syslog(LOG_ERR, "destory thread fail of chnid %d", ptr->chnid);
				return -1;
			}else{
				pthread_cancel(thr_channel[i].tid);
				pthread_join(thr_channel[i].tid, 0);
				thr_channel[i].chnid = -1;
			}
		}
	}
	return 0;
}
int thr_channel_destroyAll(){
	for(int i = 0; i<  CHNNR; i++){
		if(thr_channel[i].chnid > 0){
			if(pthread_cancel(thr_channel[i].tid) < 0){
				syslog(LOG_ERR, "destory thread fail of chnid %d", thr_channel[i].chnid);
			}else{
				pthread_join(thr_channel[i].tid,0);
				thr_channel[i].chnid = -1;
			}
		}
	}
	return 0;
}
