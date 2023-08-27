#ifndef __CLIENT_H__
#define __CLIENT_H__
#define DEFAULT_PORT "1989"
#define DEFAULT_CMD "/usr/bin/mpg123 - >/dev/null"
struct client_conf_st {
	char * rcvport;
	char * mgroup;
	char * player_cmd;
};
extern struct client_conf_st client_conf;
#endif
