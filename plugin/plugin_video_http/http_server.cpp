//http_server.cpp

#include "http_server.h"

#include <thread>
#include<iostream>
#include<vector>
#include<chrono>
using namespace std::chrono;
using namespace std;


Http_Server::Http_Server(int port, int _timeout)
    : sock_(INVALID_SOCKET)
    , port_(port)
    , timeout_(_timeout)
    , quality_(40)
    , close_all_sockets_(0)
{
    FD_ZERO(&master_);
    if (port)
        open(port);
}

Http_Server::~Http_Server()
{
    close_all();
    release();
}

void Http_Server::wait_accept()
{
   //string file_path = "cars.mp4";
   //std::thread t(&Http_Server::filedecode, this,file_path);
   //filedecode(file_path);
   int index = 0;
    while (true) {
        string file_name = "temp" + std::to_string(index)+".jpg";

        index = index % 1200;
        //cout<<"index:" <<index<<endl;
        
        FILE* fp = fopen(file_name.c_str(), "rb");
        if (fp) {
            fseek(fp, 0L, SEEK_END);
            int size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char* data = (char*)malloc(size);
            if (data) {
                fread(data, size, 1, fp);
                write(data, size);
                free(data);
            }
            fclose(fp);
            index++;
            
            /*if (index > decode_index_) {
                index = decode_index_;
            }*/
            usleep(40000);
        }
        
    }
}


void Http_Server::send_image(char* data, int size)
{
    write(data, size);
}



bool Http_Server::write(char * data, int size)
{
    // cout << "write start now" <<  endl;
    fd_set rread = master_;
    struct timeval select_timeout = { 0, 0 };
    struct timeval socket_timeout = { 0, timeout_ };
    //FD_ZERO(&rread);
    FD_SET(maxfd_, &rread);
    // cout<<"start s <= maxfd_ 0:" <<maxfd_<< endl;
    int select_res = ::select(maxfd_ + 1, &rread, NULL, NULL, &select_timeout);
    
    if (select_res <= 0)
    {
        // cout << "nothing broken, there's just noone listening: " << select_res<< endl;
        return true; // nothing broken, there's just noone listening
    }
#ifdef WIN32
    for (unsigned i = 0; i < rread.fd_count; i++) {
        int addrlen = sizeof(SOCKADDR);
        SOCKET s = rread.fd_array[i]; // fd_set on win is an array, while ...
#else
    for (int s = 0; s <= maxfd_; s++) {
        // cout<<"start s <= maxfd_:"<<maxfd_<<" socket_:"<<sock_ <<endl;
        socklen_t addrlen = sizeof(SOCKADDR);
        
        if (!FD_ISSET(s, &rread)) // ... on linux it's a bitmask ;)
        {
            // cout<<"on linux it's a bitmask" <<endl;
            continue;
        }
#endif
        if (s == sock_)  {// request on master socket, accept and send main header.
            SOCKADDR_IN address = { 0 };
            // cout<<"start accept" <<endl;
            SOCKET client = ::accept(sock_, (SOCKADDR*)&address, &addrlen);
            // cout<<"end accept" <<endl;
            
            if (client == SOCKET_ERROR)
            {
                cout << "error MJPG_sender: couldn't accept connection on sock " << sock_ << " !" << endl;
                cerr << "error MJPG_sender: couldn't accept connection on sock " << sock_ << " !" << endl;
                return false;
            }
            if (setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (char*)&socket_timeout, sizeof(socket_timeout)) < 0) {
                cerr << "error MJPG_sender: SO_RCVTIMEO setsockopt failed\n";
                cout << "error MJPG_sender: SO_RCVTIMEO setsockopt failed\n";
            }
            if (setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (char*)&socket_timeout, sizeof(socket_timeout)) < 0) {
                cerr << "error MJPG_sender: SO_SNDTIMEO setsockopt failed\n";
                cout << "error MJPG_sender: SO_SNDTIMEO setsockopt failed\n";
            }
            maxfd_ = (maxfd_ > client ? maxfd_ : client);
            FD_SET(client, &master_);
            // cout << "write head start" << endl;
            _write(client, "HTTP/1.0 200 OK\r\n", 0);
            _write(client,
                "Server: Mozarella/2.2\r\n"
                "Accept-Range: bytes\r\n"
                "Connection: close\r\n"
                "Max-Age: 0\r\n"
                "Expires: 0\r\n"
                "Cache-Control: no-cache, private\r\n"
                "Pragma: no-cache\r\n"
                "Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n"
                "Content-Length: (.*)\r\n"
                "\r\n",
                0);
            // cout << "write head end" << endl;
        } else  {                                       // existing client, just stream pix
            if (close_all_sockets_) {
                int result = close_socket(s);
                cerr << "MJPG_sender: close clinet: " << result << " \n";
                continue;
            }
            // cout << "write data start" << endl;
            char head[400] = { 0 };
            sprintf(head, "--mjpegstream\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n", (size_t)size);
            // cout<< "head:" << head<<endl;
            int res_head = _write(s, head, strlen(head));
            if(!res_head) {
                cout<<"res_head:"<<res_head<<"head_size:"<<strlen(head)<<"error"<<strerror(errno)<<endl;
                close_socket(s);
                FD_CLR(s, &master_);
            }
            int n = _write(s, (char*)data, size);
            // cout<<"res_data:"<<n<<"data_size:"<<size<<endl;
            if (n < (int)size) {
                printf("send failed %d (%d vs %d) %s\n", s, n, size, strerror(errno));
                close_socket(s);
                FD_CLR(s, &master_);
                FD_ZERO(&master_);
                open(port_);
            }
            // cout << "write data end" << endl;
        }
        
    }
    if (close_all_sockets_) {
        int result = close_socket(sock_);
        cerr << "MJPG_sender: close acceptor: " << result << " \n\n";
    }
    return true;
}

int Http_Server::_write(int fd, char const* buffer, int length)
{
    if (length < 1) { 
        length = strlen(buffer); 
    }
    int bytes_left = length;
    int written_bytes = 0;
    char* ptr = (char*)buffer;
    try{
        while (bytes_left > 0) {
            signal(SIGPIPE, SIG_IGN);  // 忽略SIGPIPE信号，防止程序退出
            written_bytes = ::send(fd, ptr, bytes_left, 0);
            if (written_bytes <= 0)  {
                printf("written_bytes [%d]\n", written_bytes);
                /* ������*/
                if (errno == EINTR || errno == EAGAIN) {
                    printf("[%s]:error errno==EINTR continue\n", __FUNCTION__);
#ifdef WIN32
                    Sleep(1);
#else
                    usleep(100000);
#endif             
                    continue;
//                 } else if (errno == EPIPE) {
//                     printf("[%s]:error errno==EPIPE return\n", __FUNCTION__);
// #ifdef WIN32
//                     Sleep(1);
// #else
//                     usleep(100);
// #endif             
//                     continue;
                } else {
                    printf("written_bytes break [%d],[%d]\n", written_bytes, bytes_left);
                    printf("send failed %s, %d \n", strerror(errno), errno);
                    break;
                    // usleep(100);
                    // continue;
                }
            } else {
                ptr += written_bytes;
                bytes_left -= written_bytes;
            }
        }
    }
    catch(...)  {
          cout << " Error in _write function \n";
    }
    return length - bytes_left;
}
bool Http_Server::open(int port)
{
    /*
    //��ʼ��WSA  
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    if (WSAStartup(sockVersion, &wsaData) != 0) {
        return release();
    }*/

    sock_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN address;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_family = AF_INET;
    address.sin_port = htons(port); // ::htons(port);
    int reuse = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0)
        cerr << "setsockopt(SO_REUSEADDR) failed" << endl;

    // Non-blocking sockets
    // Windows: ioctlsocket() and FIONBIO
    // Linux: fcntl() and O_NONBLOCK
#ifdef WIN32
    unsigned long i_mode = 1;
    int result = ioctlsocket(sock_, FIONBIO, &i_mode);
    if (result != NO_ERROR) {
        std::cerr << "ioctlsocket(FIONBIO) failed with error: " << result << std::endl;
    }
#else  // WIN32
    int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
#endif // WIN32

#ifdef SO_REUSEPORT
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)) < 0)
        cerr << "setsockopt(SO_REUSEPORT) failed" << endl;
#endif
    if (::bind(sock_, (SOCKADDR*)&address, sizeof(SOCKADDR_IN)) == SOCKET_ERROR) {
        cerr << "error MJPG_sender: couldn't bind sock " << sock_ << " to port " << port << "!" << endl;
        return release();
    }
    if (::listen(sock_, 10) == SOCKET_ERROR) {
        cerr << "error MJPG_sender: couldn't listen on sock " << sock_ << " on port " << port << " !" << endl;
        return release();
    }
    FD_ZERO(&master_);
    FD_SET(sock_, &master_);
    maxfd_ = sock_;
    cout<<"open sucess:maxfd_:"<<maxfd_<<endl;
    return true;
    
}

bool Http_Server::release()
{
    if (sock_ != INVALID_SOCKET)
        ::shutdown(sock_, 2);
    sock_ = (INVALID_SOCKET);
    return false;
}

void Http_Server::close_all()
{
    close_all_sockets_ = 1;
}

int Http_Server::close_socket(SOCKET s)
{
    // int close_output = ::shutdown(s, 1); // 0 close input, 1 close output, 2 close both
    char* buf = (char*)calloc(1024, sizeof(char));
    ::recv(s, buf, 1024, 0);
    free(buf);
    // int close_input = ::shutdown(s, 0);
#ifdef WIN32
    int result = closesocket(s);
#else
    int result = ::close(s);
#endif
    //int result = ::close(s);
    //std::cerr << "Close socket: out = " << close_output << ", in = " << close_input << " \n";
    return result;
}
