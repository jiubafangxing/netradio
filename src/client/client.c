#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <getopt.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/errno.h>
#include "../include/proto.h"
#include "client.h"
/**
 *
 *
 * **/
struct client_conf_st myconf = {\
	.mgroup = DEFAULT_MGROUP,\
		.player_cmd = DEFAULT_CMD,\
		.rcvport = DEFAULT_PORT

};

int writen(int fd , const char * buf, size_t len){
	int pos = 0;
	while(len > 0 ){
		int writeLen = 	write(fd, buf+pos, len);
		if(writeLen <  0){
			if(errno == EAGAIN){
				continue;
			}
			return -1;

		}
		len -= writeLen;
		pos += writeLen;
	}
	return 0;

}
void printhelp(){
	printf("--port -P to bind port \n\
			--mgroup -M to bind group\n\
			--player -p to bind player program\n\
			--help -h to print help	");
}
int main(int argc, char * argv[]){
	/**
	 * 初始化
	 * 级别 : 默认值，配置文件，环境变量，命令行参数
	 **/
	int pd[2];
	struct option  options[] = {{"port",1,NULL, 'P'},{"mgroup",1,NULL,'M'},{"player",1,NULL, 'p'},{"help",1, NULL, 'h'},{NULL,0,NULL, 0}};
	int index = 0;
	while(1){
		int parseResult = getopt_long(argc, argv, "P:M:p:h",options,&index);
		if(parseResult < 0){
			break;
		}
		switch(parseResult){
			case 'P':
				myconf.rcvport = optarg;
				break;
			case 'M':
				myconf.mgroup = optarg;
				break;
			case 'p':
				myconf.player_cmd = optarg;
				break;
			case 'h':
				printhelp();
				break;
		}
	}

	int fd = 	socket(AF_INET,SOCK_DGRAM,0);
	if(fd < 0){
		perror("socket");
		exit(1);
	}
	struct ip_mreqn req;
	struct in_addr groupaddr;
	struct in_addr myaddr;
	printf("%s\n",myconf.mgroup);
	printf("%s\n",myconf.rcvport);
	inet_pton(AF_INET,myconf.mgroup,&req.imr_multiaddr.s_addr);

	inet_pton(AF_INET,"0.0.0.0",&req.imr_address.s_addr);
	int nameindex = if_nametoindex("wlp3s0");
	req.imr_ifindex = nameindex;
	int setIpoptResult = setsockopt(fd, IPPROTO_IP,IP_ADD_MEMBERSHIP, &req,sizeof(req));
	if(setIpoptResult <  0){
		perror("socket");
		exit(1);
	}
	int openloop = 1;
//	int loopoptresult = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP,&openloop, sizeof(openloop));
//	if(loopoptresult < 0){
//		perror("loopoptresult ");
//		exit(1);
//	}
	struct sockaddr_in  bindaddr, serveraddress, remoteAddress;
	memset(&bindaddr,0,sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_port = htons( atoi(myconf.rcvport));
	inet_pton (AF_INET,"0.0.0.0",&bindaddr.sin_addr.s_addr);
	int bindResult = bind(fd,(struct sockaddr *)&bindaddr, sizeof(bindaddr)); 	
	if(bindResult <  0){
		perror("bind()");
		exit(1);
	}
	int pdresult = pipe(pd);
	if(pdresult < 0){
		perror("pd error");
		exit(1);
	}
	int forkResult = fork();
	if(forkResult <  0){
		perror("fork err");
		exit(1);
	}
	if(forkResult == 0){
		//子进程
		//调用解码器
		close(fd);
		close(pd[1]);
		dup2(pd[0],0);
		execl("/usr/bin/bash","sh","-c", myconf.player_cmd, NULL);
		exit(1);
	}else{
		//收包， 发给子进程
		struct msg_list_st * mls;
		mls = malloc(MSG_CHANNEL_MAX);
		if(mls == NULL){
		 perror("malloc");
		 exit(1);
		}
		socklen_t slt = sizeof(serveraddress); 
		int len;
		while(1){
			len = recvfrom(fd, mls, MSG_CHANNEL_MAX,0,(void *)&serveraddress,&slt );
			printf("receive msg end %d\n", len);
			if(len < sizeof(struct msg_list_st)){
				fprintf(stderr,"message is too small\n");
				continue;
			}
			printf("chnid  %d", mls->chnid);
			if(mls->chnid != LISTCHNID){
				fprintf(stderr, "chnid is not match\n");
				continue;
			}
		printf("print channel info \n");	
			break;
		}
		struct msg_listentry_st * pos;
		for(pos= mls->entry; (char *)pos <(((char *)mls) + len)	;pos =  (void *)(((char *)pos) + ntohs(pos->len))){
			printf("channelId: %d : desc: %s\n",pos->chnid,pos->desc);
		}
		free(mls);
		int choosen;
		//收节目单
		while(1){
			int inputresult = scanf("%d", &choosen);
			if(inputresult !=   1){
				perror("scanf err");
				exit(1);
			}
			break;
		}
		struct msg_channel_st *msgChannel;
		msgChannel = malloc(MSG_CHANNEL_MAX);
		socklen_t remoteAddressAttr = sizeof(remoteAddress); 
		while(1){
			//选择频道
			int recvResult  =	recvfrom(fd, msgChannel,MSG_CHANNEL_MAX,0,(void *)&remoteAddress,&remoteAddressAttr ); 
			
			if(recvResult <  0)	{
				perror("recv error");
				continue;
			}
			if(remoteAddress.sin_addr.s_addr != serveraddress.sin_addr.s_addr){
				perror("sin_addr.s_addr error ");
				continue;	
			}
			if(remoteAddress.sin_port != serveraddress.sin_port ){
				perror("remote addr error");
				continue;
			}
			if(remoteAddressAttr != slt){
				perror("add len err");
				continue;
			}
			
				printf("datais %ld \n",sizeof(msgChannel->data));
			if(msgChannel->chnid == choosen){
				printf("datais %ld \n",sizeof(msgChannel->data));
				int pipeWriteResult = 	writen(pd[1],(void *)msgChannel->data,recvResult -sizeof(msgChannel->data));
				if(pipeWriteResult <  0){
					exit(-1);
				}
			}	
			//收频道包，发送给子进程
		}
		free(msgChannel);
		exit(0);
	}



	exit(1);
}
