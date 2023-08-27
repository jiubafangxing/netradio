#include "mytbf.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
struct mytbf_st {
	int cps;
	int burst;
	int token;
	int pos;
	pthread_cond_t cond;
	pthread_mutex_t mutex;

};
static struct mytbf_st *job[MYTBF_MAX];
static pthread_mutex_t poolmutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_once_t once = PTHREAD_ONCE_INIT;
static pthread_t pt;
int getFreePosition_unlock(){
	for(int i = 0; i< MYTBF_MAX; i++){
		if(job[i] == NULL){
			return i;
		}
	}
	return -1;
}
static void * thread_start(void *arg){
	struct timespec ti;
	ti.tv_sec = 1;
	ti.tv_nsec = 0;
	while(1){
		pthread_mutex_lock(&poolmutex);
		for(int i = 0; i< MYTBF_MAX; i++){
			if(NULL != job[i]){
				pthread_mutex_lock(&job[i]->mutex);
				job[i]->token += job[i]->cps;
				if(job[i]->token >job[i]->burst){
					job[i]->token = job[i]->burst;
				}	
				pthread_cond_broadcast(&job[i]->cond);
				pthread_mutex_unlock(&job[i]->mutex);	
			}	
		}
		pthread_mutex_unlock(&poolmutex);
		usleep(50000);
	}
}
static void moduleunload(){
	pthread_join(pt,NULL);
	pthread_cancel(pt);
	for(int i = 0; i< MYTBF_MAX; i++){
		if(NULL != job[i]){
			mytbf_destory(job[i]);
		}	
	}

}
static void moduleload(void ){
	pthread_create(&pt,NULL,thread_start, NULL);
	atexit(moduleunload);
}
mytbf_t * mytbf_init(int cps, int burst){
	struct mytbf_st  *mytbf;
	mytbf = malloc(sizeof(*mytbf));
	mytbf->cps = cps;
	mytbf->burst = burst;	
	pthread_once(&once,moduleload);
	pthread_mutex_lock(&poolmutex);
	int pos = getFreePosition_unlock();
	mytbf->pos = pos;
	job[pos] = mytbf;
	pthread_mutex_unlock(&poolmutex);
	pthread_mutex_init(&mytbf->mutex,NULL);
	pthread_cond_init(&mytbf->cond,NULL);
	return (void *)mytbf;
}
int min(int c1, int c2){
	if(c1 > c2){
		return c2;
	}else{
		return c1;
	}
}

int mytbf_fetchtoken(mytbf_t * mytbft, int size ){
	struct mytbf_st * mytbf = mytbft;
	pthread_mutex_lock(&mytbf->mutex);	
	while(mytbf->token <= 0){
		pthread_cond_wait(&mytbf->cond, &mytbf->mutex);
	}
	int returnsize = min(mytbf->token,size);	
	mytbf->token -= returnsize;
	pthread_mutex_unlock(&mytbf->mutex);	
	return returnsize;
}
int mytbf_returntoken(mytbf_t * mytbft, int size ){
	struct mytbf_st * mytbf = mytbft;
	pthread_mutex_lock(&mytbf->mutex);	
	mytbf->token = mytbf->token + size;
	if(mytbf->token > mytbf->burst){
		mytbf->token = mytbf->burst;
	}
	pthread_cond_broadcast(&mytbf->cond);
	pthread_mutex_unlock(&mytbf->mutex);	
	return 0;
}
int mytbf_destory(mytbf_t * mytbft){
	struct mytbf_st * mytbf = mytbft;
	pthread_mutex_destroy(&mytbf->mutex);
	pthread_cond_destroy(&mytbf->cond);
	pthread_mutex_lock(&poolmutex);
	job[mytbf->pos] = NULL;
	pthread_mutex_unlock(&poolmutex);
	free(mytbf);
	return 0;
}
