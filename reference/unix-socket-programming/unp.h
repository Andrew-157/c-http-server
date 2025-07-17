#ifndef __unp_h
#define __unp_h

#include "config.h" /* configuration options for current OS */

#include <sys/types.h>  /* basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <sys/time.h>   /* timeval{} for select */
#include <time.h>       /* timespec{} for pselect() */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h>  /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>      /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  /* for S_xxx file mode constants */
#include <sys/uio.h>   /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h>    /* for Unix domain sockets */

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>  /* for convenience */
#endif

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>  /* for convenience */
#endif

#ifdef HAVE_SYS_EVENT_H
#include <sys/event.h>  /* for kqueue */
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>  /* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 * */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#ifdef HAVE_PTHREAD_H
#include <pthread.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#ifdef HAVE_NETINET_SCTP_H
#include <netinet/sctp.h>
#endif

/* OSF/1 actually disables recv() and send() in <sys/socket.h> */
#ifdef __osf__
#undef recv
#under send
#define recv(a,b,c,d) recvfrom(a,b,c,d,0,0)
#define send(a,b,c,d) sendto(a,b,c,d,0,0)
#endif


