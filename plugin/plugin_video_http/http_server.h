//http_server.h

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#ifdef WIN32
#include <winsock2.h>
#include<windows.h>
#pragma comment(lib,"ws2_32.lib") 
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#endif
#include <string>
#include "string.h"
//#define SOCKET int 

#define PORT unsigned short
#define SOCKET int
#define HOSTENT struct hostent
#define SOCKADDR struct sockaddr
#define SOCKADDR_IN struct sockaddr_in
#define ADDRPOINTER unsigned int *
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif


class Http_Server {
public:
	 Http_Server(int port = 0, int _timeout = 400000);
	 ~Http_Server();
     void wait_accept();
     void send_image(char * data,int size);
private:
    bool open(int port);
    bool write(char * data, int size);
    int _write(int fd, char const* buffer, int length);
    bool release();
    void close_all();
    int close_socket(SOCKET s);
    int filedecode(std::string filename);
private:
    SOCKET sock_;
    SOCKET maxfd_;
    fd_set master_;
    int port_;
    int timeout_; // master sock timeout, shutdown after timeout usec.
    int quality_; // jpeg compression [1..100]
    int close_all_sockets_;
    int decode_index_;

};
#endif //HTTP_SERVER_H
