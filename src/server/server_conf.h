#ifndef __SERVER_CONF_H__
#define __SERVER_CONF_H__
struct server_const_st{
	char *rcvport;
	char *mgroup;
	char *media_dir;
	char runmode;
	char *ifname;

};
extern struct server_const_st serverConf;
extern int serversd;
extern struct sockaddr_in sndaddr;
#endif
