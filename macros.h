#ifndef FWIO_MACROS_H
#define FWIO_MACROS_H

#include <err.h>
#define ERR_IF(t,msg) if((t)) err(EXIT_FAILURE,"%s",msg);

#define STATICBUF(structname, buftype, buflen) \
	struct structname##_t { buftype space[buflen]; unsigned int next; }

#define epoll_init() do { \
    assert(epoll_create1(EPOLL_CLOEXEC) == EPOLL_FD); \
} while(0)


#endif
