//
// Created by jerome on 11/23/23.
//

#include "tcp-utils.h"
#include <sys/socket.h>
#include <netdb.h>
#include <sys/poll.h>
#include <unistd.h>
#include <cstring>


/*
 * Miscelanious TCP (and general) functions.  Contains functions that
 * connect active sockets, put passive sockets in listening mode, and
 * read from streams.
 *
 * By Stefan Bruda, using the textbook as inspiration.
 */

#include "tcp-utils.h"

const int err_host    = -1;
const int err_sock    = -2;
const int err_connect = -3;
const int err_proto   = -4;
const int err_bind    = -5;
const int err_listen  = -6;
const int nodata = -2;

int connectbyport(const char* host, const char* port) {
    return connectbyportint(host,(unsigned short)atoi(port));
}

int connectbyservice(const char* host, const char* service){
    struct servent* sinfo = getservbyname(service,"tcp");  // service info
    if (sinfo == NULL)
        return err_proto;
    return connectbyportint(host,(unsigned short)sinfo->s_port);
}

int connectbyportint(const char* host, const unsigned short port) {
    struct hostent *hinfo;         // host information
    struct sockaddr_in sin;        // address to connect to
    int sd;                        // socket descriptor to be returned
    const int type = SOCK_STREAM;  // TCP connection

    memset(&sin, 0, sizeof(sin));  // needed for correct padding... (?)
    sin.sin_family = AF_INET;

    // host name to IP address
    hinfo = gethostbyname(host);
    if (hinfo == NULL)
        return err_host;
    memcpy(&sin.sin_addr, hinfo->h_addr, hinfo->h_length);

    sin.sin_port = (unsigned short)htons(port);

    // allocate socket:
    sd = socket(PF_INET, type, 0);
    if ( sd < 0 )
        return err_sock;

    // connect socket:
    int rc = connect(sd, (struct sockaddr *)&sin, sizeof(sin));
    if (rc < 0) {
        close(sd);
        return err_connect;
    }

    // done!
    return sd;
}

int passivesocketstr(const char* port, const int backlog) {
    return passivesocket((unsigned short)atoi(port), backlog);
}

int passivesocketserv(const char* service, const int backlog) {
    struct servent* sinfo = getservbyname(service,"tcp");  // service info
    if (sinfo == NULL)
        return err_proto;
    return passivesocket((unsigned short)sinfo->s_port, backlog);
}

/*
 * Helper function, contains the common code for passivesocket and
 * controlsocket (which are identical except for the IP address they
 * bind to).
 */
int passivesockaux(const unsigned short port, const int backlog, const unsigned long int ip_addr) {
    struct sockaddr_in sin;        // address to connect to
    int	sd;                        // socket description to be returned
    const int type = SOCK_STREAM;  // TCP connection

    memset(&sin, 0, sizeof(sin));  // needed for correct padding... (?)
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(ip_addr);

    sin.sin_port = (unsigned short)htons(port);

    // allocate socket:
    sd = socket(PF_INET, type, 0);
    if ( sd < 0 )
        return err_sock;

    // bind socket:
    if ( bind(sd, (struct sockaddr *)&sin, sizeof(sin)) < 0 ) {
        close(sd);
        return err_bind;
    }
    // socket is listening:
    if ( listen(sd, backlog) < 0 ) {
        close(sd);
        return err_listen;
    }

    // done!
    return sd;
}

int passivesocket(const unsigned short port, const int backlog) {
    return passivesockaux(port, backlog, INADDR_ANY);
}

int controlsocket(const unsigned short port, const int backlog) {
    return passivesockaux(port, backlog, INADDR_LOOPBACK);
}

int readline(const int fd, char* buf, const size_t max) {
    size_t i;
    int begin = 1;

    for (i = 0; i < max - 2; i++) {
        char tmp;
        int what = read(fd,&tmp,1);
        if (what == -1)
            return -1;
        if (begin) {
            if (what == 0)
                return recv_nodata;
            begin = 0;
        }
        if (what == 0 || tmp == '\n' || tmp == '\r') {
            if(tmp = '\r')
                buf[i+1] = '\0';
            buf[i] = '\0';
            return i;
        }
        buf[i] = tmp;
    }
    buf[i] = '\0';
    return i;
}