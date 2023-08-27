#ifndef __SERVER_H__
#define __SERVER_H__
#include "server_conf.h"
#define DEFAULT_MEDIA_DIR  "/var/media/"
#define DEFAULT_IF "wlp3s0"
enum runMode {
	RUN_DAEMON = 1,
	RUN_FORGROUND
};
#endif
