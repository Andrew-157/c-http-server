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

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff /* should have been in <netinet/in.h> */
#endif

#ifndef SHUT_RD      /* these three POSIX names are new */
#define SHUT_RD 0    /* shutdown for reading */
#define SHUT_WR 1    /* shutdown for writing */
#define SHUT_RDWR 2  /* shutdown for reading and writing */
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16 /* "ddd.ddd.ddd.ddd\0"
                               1234567890123456 */
#endif

/* Define following even if IPv6 not supported, so we can always allocate an adequately
 sized buffer without #ifdefs in the code. */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46 /* max size of IPv6 address string:
                            "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx" or
                            "xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:ddd.ddd.ddd.ddd\0"
                            1234567890123456789012345678901234567890123456 */
#endif

/* Define bzero as a macro if it's not supported in standard C library */
#ifndef HAVE_BZERO
#define bzero(ptr,n) memset(ptr, 0, n)
#endif

/* Older resolvers do not have gethostbyname2() */
#ifndef HAVE_GETHOSTBYNAME2
#define gethostbyname2(host,family)  gethostbyname((host))
#endif

/* the structure returned by recvfrom_flags() */
struct unp_in_pktinfo {
    struct in_addr ipi_addr; /* dst IPv4 address*/
    int ipi_ifindex;         /* received interface index */
};

/* We need the newer CMSG_LEN() and CMSG_SPACE() macros, but few implementations
 support them today . These two macros really need an ALIGN() macro, but each
 implementation does this differently. */
#ifndef CMSG_LEN
#define CMSG_LEN(size) (sizeof(struct cmsghdr) + (size))
#endif
#ifndef CMSG_SPACE
#define CMSG_SPACE(size) (sizeof(struct cmsghdr) + (size))
#endif

/* POSIX requires the SUN_LEN() macro, but not all implementations define it (yet). 
Note that this 4.4BSD macro works regardless whether there is a length field or not. */
#ifndef SUN_LEN
#define SUN_LEN(su) \
    (sizeof(*(su)) - sizeof((su)->sun_path) + strlen((su)->sun_path))
#endif

/* POSIX renames "Unix domain" as "local IPC."
 Not all systems define AP_LOCAL and PF_LOCAL (yet). */
#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif
#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

/* POSIX requires that an #include of <poll.h> define INFTIM, but many
systems still define it in <sys/stropts.h>. We don't want to include
all the STREAMS stuff if it's not needed, so we just define INFTIM here.
This is the standard value, but there is no guarantee it is -1. */
#ifndef INFTIM
#define INFTIM (-1) /* infinite poll timeout */
#ifdef HAVE_POLL_H
#define INFTIM_UNPH /* tell unpxti.h we defined it */
#endif
#endif

/* Following could be derived from SOMAXCONN in <sys/socket.h>, but many
kernels still #define it as 5, while actually supporting many more */
#define LISTENQ 1024 /* 2nd argument to listen() */

/* Miscellaneous constants */
#define MAXLINE  4096 /* max text line length */
#define BUFFSIZE 8192 /* buffer size for reads and writes */

/* define some port number that can be used for our examples */
#define SERV_PORT      9877            /* TCP and UDP */
#define SERV_PORT_STR "9877"           /* TCP and UDP */
#define UNIXSTR_PATH  "/tmp/unix.str"  /* Unix domain stream */
#define UNIXDG_PATH   "/tmp/unix.dg"   /* Unix domain datagram */

/* Following shortens all the typecasts of pointer arguments: */
#define SA struct sockaddr

/* #define HAVE_STRUCT_SOCKADDR_STORAGE - raises a warning because already defined in config.h */
#ifndef HAVE_STRUCT_SOCKADDR_STORAGE
/*
 * RFC 3493: protocol-independent placeholder for socket addresses
 */
#define __SS_MAXSIZE   128
#define __SS_ALIGNSIZE (sizeof(int64_t))
#ifdef HAVE_SOCKADDR_SA_LEN
#define __SS_PAD1SIZE (__SA_ALIGNSIZE - sizeof(u_char) - sizeof(sa_family_t))
#else
#define __SS_PAD1SIZE (__SS_ALIGNSIZE - sizeof(sa_family_t))
#endif
#define __SS_PAD2SIZE (__SS_MAXSIZE - 2*__SS_ALIGNSIZE)

struct sockaddr_storage {
#ifdef HAVE_SOCKADDR_SA_LEN
    u_char ss_len;
#endif
    sa_family_t ss_family;
    char        __ss_pad1[__SS_PAD1SIZE];
    int64_t     __ss_align;
    char        __ss_pad2[__SS_PAD2SIZE];
};
#endif

#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
                  /* default file access permissions for new files */
#define DIR_MODE  (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
                  /* default permissions for new directories */

typedef void Sigfunc (int); /* for signal handlers */

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

#ifndef HAVE_ADDRINFO_STRUCT
#include "../lib/addrinfo.h" // HAVE_ADDRINFO_STRUCT is set to 1 on my system, so it doesn't matter
#endif

#ifndef HAVE_IF_NAMEINDEX_STRUCT
struct if_nameindex {
    unsigned int if_index;  /* 1, 2, ... */
    char     *if_name;          /* null-terminated name: "le0", ... */
};
#endif

#ifndef HAVE_TIMESPEC_STRUCT
struct timespec {
    time_t tv_sec;     /* seconds */
    long   tv_nsec;    /* and nanoseconds */
};
#endif

#endif /* __unp_h */
