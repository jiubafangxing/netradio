#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "../include/proto.h"
#include "medialib.h"
#include "thr_list.h"
#include "server_conf.h"
static pthread_t tid_list;
static int nr_list_ent;
static struct mlib_listentry_st *list_ent;
struct mliblist_st {
 	struct mlib_listentry_st  *mlibst;
	size_t size;

};
void * call(void *p){
	int totalsize;
	struct msg_list_st *entrylistp;
	struct msg_listentry_st *entryp;
	totalsize = sizeof(chnid_t);
	for(int i = 0; i< nr_list_ent; i++){
		totalsize += sizeof(struct msg_listentry_st)+strlen(list_ent[i].desc);
	}
	entrylistp = malloc(totalsize);
	if(entrylistp == NULL){
		syslog(LOG_ERR, "malloc fail");
		exit(1);
	}
	entrylistp->chnid = LISTCHNID;
	entryp = entrylistp->entry;
	int size = 0;
	for (int i = 0; i< nr_list_ent; i++){
		size = sizeof(struct msg_listentry_st)+ strlen(list_ent[i].desc);
		entryp->chnid = list_ent[i].chnid;
		entryp->len = htons(size);
		strcpy((void *)entryp->desc, list_ent[i].desc);
		entryp =(void *)( ((char *)entryp) +size);
	}
	while(1){

		int ret = 		sendto(serversd,entrylistp,totalsize, 0,(void *) &sndaddr, sizeof(sndaddr));
		if(ret <  0){
			syslog(LOG_ERR, "send to entrylist err..");
		}
		else{
			syslog(LOG_DEBUG,"send to entrylist success %d",ret );
		}
		sleep(1);
	}


}


size_t countsize(struct mlib_listentry_st * listst, size_t arraysize){
	int len = 0;
	for(int i = 0; i< arraysize ; i++){
	
		len+= sizeof(listst[i].desc)+sizeof(chnid_t);
	}
	return len;
}
int thr_list_create(struct mlib_listentry_st * mlibst, size_t size){
	list_ent = mlibst;
	nr_list_ent = size;
	pthread_create(&tid_list,NULL,call,NULL);
}
int thr_list_destory(){
	   pthread_join(tid_list,0);
	   pthread_cancel(tid_list);	   
}
