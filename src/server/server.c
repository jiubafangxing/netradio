#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "server.h"
#include  "thr_list.h"
#include  "thr_channel.h"
#include  <proto.h>


int serversd;
struct sockaddr_in sndaddr;

static struct mlib_listentry_st *  mlibst;
/***
 *
 * -M 指定多播组
 * -P 指定接受端口
 * -F 前台运行
 * -D 指定媒体库位置
 * -H 显示帮助
 *
 * ***/
struct server_const_st serverConf = {
	.ifname =DEFAULT_IF,
	//	.media_dir = DEFAULT_MEDIA_DIR,
	.media_dir ="/home/zed/Music",
	.mgroup = DEFAULT_MGROUP,
	.rcvport = DEFAULT_RCVPORT,
	.runmode = RUN_DAEMON

};

int socket_init(){
	serversd= 	socket(AF_INET,SOCK_DGRAM, 0);
	struct sockaddr_in serv;
	serv.sin_addr.s_addr = htonl(INADDR_ANY);
	serv.sin_port = htons(8787);
	serv.sin_family = AF_INET;
	int ret = bind(serversd, (struct sockaddr *) &serv, sizeof(serv));
	if(ret == -1){
		perror("bind err");
		exit(1);
	}
	struct ip_mreqn mreqn;
	struct sockaddr_in groupaddr;
	struct sockaddr_in  myaddr;
	int mgroupresult = inet_pton(AF_INET ,serverConf.mgroup,&groupaddr.sin_addr.s_addr);
	if(mgroupresult < 0){
		syslog(LOG_ERR, "inet_pton gourp fail");
		return -1;
	}
	int myaddrresult = inet_pton(AF_INET ,"0.0.0.0",&myaddr.sin_addr.s_addr);
	if(myaddrresult < 0){
		syslog(LOG_ERR, "inet_pton group fail");
		return -1;
	}
	mreqn.imr_address = myaddr.sin_addr;
	mreqn.imr_multiaddr = groupaddr.sin_addr;
	mreqn.imr_ifindex =if_nametoindex(serverConf.ifname);

	sndaddr.sin_family = AF_INET;
	sndaddr.sin_port =htons(atoi(serverConf.rcvport));
	printf("sndaddr port is %s -- %d",serverConf.rcvport,sndaddr.sin_port);
	inet_pton(AF_INET, serverConf.mgroup, &sndaddr.sin_addr.s_addr);
	setsockopt(serversd, IPPROTO_IP,IP_MULTICAST_IF,&mreqn, sizeof(mreqn));
	return 0;
}

void printHelp(){
	syslog(LOG_INFO," -M 指定多播组");
	syslog(LOG_INFO," -P 指定接受端口");
	syslog(LOG_INFO," -F 前台运行");
	syslog(LOG_INFO," -D 指定媒体位置");
	syslog(LOG_INFO," -H 显示帮助");
}

int daemonize(){
	int forkResult  = fork();
	if(forkResult < 0){
		syslog(LOG_ERR,"fork err");
		return -1;
	}
	if(forkResult > 0){
		exit(0);
	}else{
		int fd= open("/dev/null",O_RDWR);
		if(fd < 1){
			syslog(LOG_ERR, "open fail");
		}
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		if(fd >  2){
			close(fd);
		}
		setsid();
		chdir("/");
		return 0;
	}
}

static void exitProcess(int s){
	thr_channel_destroyAll();
	thr_list_destory();
	mlib_freechnlist(mlibst);
	syslog(LOG_WARNING,"signal-%d caught, exit, now", s);
	closelog();
	exit(0);
}
int main(int argc, char * argv[]){
	openlog("netradio",LOG_PERROR|LOG_PID,LOG_DAEMON);
	struct sigaction  myaction;
	myaction.sa_handler = exitProcess;
	sigemptyset(&myaction.sa_mask);	
	sigaddset(&myaction.sa_mask,SIGTERM);
	sigaddset(&myaction.sa_mask,SIGQUIT);
	sigaddset(&myaction.sa_mask,SIGINT);
	sigaction(SIGTERM, &myaction, NULL);
	sigaction(SIGINT,&myaction, NULL);
	sigaction(SIGQUIT,&myaction, NULL);
	/**命令行分析**/
	int opt;
	while(opt != -1){
		opt = getopt(argc, argv,"M:P:FD:H");
		switch(opt){
			case 'M':
				serverConf.mgroup = optarg;
				break;
			case 'P':
				serverConf.rcvport = optarg;
				break;
			case 'F':
				serverConf.runmode= RUN_FORGROUND;
				break;
			case 'D':
				serverConf.media_dir = optarg;
				break;
			case 'H':
				printHelp();
				break;
		}
	}
	/**守护进程的实现**/
	if(serverConf.runmode == RUN_DAEMON){
		int daemonResult = daemonize();
		if(daemonResult <  0){
			perror("daemonize err");
			exit(1);
		}
	}else if (serverConf.runmode == RUN_FORGROUND){
		printf("前台\n");
	}else{
		syslog(LOG_ERR, "err run mode ");
		exit(1);
	}
	/**socket 初始化**/
	int socketfd = socket_init();		
	if(socketfd < 0){
		syslog(LOG_ERR, "socket_init fail ");
		exit(1);
	}

	int listSize;
	int err;
	err  = mlib_getchnlist(&mlibst,&listSize);
	if(err <  0){
		syslog(LOG_ERR, "mlib_getchnlist fail ");
		exit(1);
	}else{
		/** 创建节目单线程**/
		err = thr_list_create(mlibst,listSize);
		if(err){
			syslog(LOG_ERR, "fail to create channel thread");
			exit(1);
		}else{

			for(int i = 0; i < listSize; i++){
//				printf("channel create file %s\n",(mlibst+i) -> desc);
				thr_channel_create(mlibst + i);
				/** 创建频道线程**/
			}
		}
	}	
	/** 创建频道线程**/
	return 0;
}
