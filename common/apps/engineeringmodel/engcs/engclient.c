#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include "stdlib.h"
#include "unistd.h"
#include <errno.h>

#include "engopt.h"
#include "engclient.h"
#include <cutils/sockets.h>

#if 0	
/*get from socket_loopback_client*/
/* Connect to port on the loopback IP interface. type is
 * SOCK_STREAM or SOCK_DGRAM. 
 * return is a file descriptor or -1 on error
 */
int eng_client(int port, int type)
{
	struct sockaddr_in addr;
	socklen_t alen;
	int s;
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	s = socket(AF_INET, type, 0);
	if(s < 0) return -1;

	if(connect(s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
	    close(s);
	    return -1;
	}

	return s;
}
#endif

int eng_client(int port, int type)
{
    int fd;
    fd = socket_local_client( "engineeringmodel",
                         0, SOCK_STREAM);
    if (fd < 0) {
        ALOGD("eng_client Unable to bind socket errno:%d[%s]", errno, strerror(errno));
    }

    return fd;
}
