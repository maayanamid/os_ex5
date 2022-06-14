//
// Created by eliannar on 12/06/2022.
//

#include <iostream>
#include<sched.h>
#include<unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/timeb.h>

#define SYS_ERROR "system error: "
#define ARG_NUM_ERROR "not enough arguments given"
#define INVALID_ARG "invalid argument given"
#define SOCKET_ERROR "couldn't open socket"
#define BIND_ERROR "couldn't bind"
#define HOSTNAME_ERROR "couldn't get hostname"
#define ACCEPT_ERROR "accept connection failed"
#define CONNECT_ERROR "creating connection failed"
#define WRITE_ERROR "problem writing"
#define INVALID_COMMAND "problem executing system command"


#define BUFFLEN 256
#define MAX_CLIENTS 5
#define SUCCESS 0
#define EXIT_FAIL 1
#define MAXHOSTNAME 40


int read_data(int s, char *buf, int n) {
    int bcount = 0;
    int br = 0;

    while (bcount < n) {
        br = read(s, buf, n-bcount);
        if (br > 0)  {
            bcount += br;
            buf += br;
        }
        if (br < 1) {
            return(br);
        }
    }
    return(bcount);
}

int write_data(int s, char* buf, int n)
{
    int bcount = 0;
    int br = 0;

    while (bcount < n) {
        br = write(s,buf,n-bcount);
        if (br > 0) {
          bcount += br;
          buf += br;
        }
        if (br < 0) {
          std::cerr << SYS_ERROR << WRITE_ERROR << std::endl;
          exit(EXIT_FAIL);
        }
    }
    return(bcount);
}


int establish(int port) {
    char myname[MAXHOSTNAME+1];
    struct sockaddr_in sa;
    struct hostent *hp;

    //hostnet initialization
    gethostname(myname, MAXHOSTNAME);
    hp = gethostbyname(myname);
    if (hp == nullptr) {
        std::cerr << SYS_ERROR << HOSTNAME_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    //sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
    // host address
    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
    // port number
    sa.sin_port= htons(port);
    // create socketf
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < SUCCESS) {
        std::cerr << SYS_ERROR << SOCKET_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    if (bind(s , (struct sockaddr *)&sa , sizeof(struct sockaddr_in)) < SUCCESS) {
        close(s);
        std::cerr << SYS_ERROR << BIND_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    listen(s, MAX_CLIENTS);
    return(s);
}


int call_socket(char *hostname, int portnum) {
    struct sockaddr_in sa;
    struct hostent *hp;

    hp = gethostbyname(hostname);
    if (hp == nullptr) {
        std::cerr << SYS_ERROR << HOSTNAME_ERROR << std::endl;
        exit(EXIT_FAIL);
    }

    bzero(&sa,sizeof(sa));
    // set address
    bcopy(hp->h_addr,(char *)&sa.sin_addr,hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons((u_short)portnum);
    // get socket
    int s = socket(hp->h_addrtype,SOCK_STREAM,0);
    if (s < SUCCESS) {
        std::cerr << SYS_ERROR << SOCKET_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    // connect to socket
    if (connect(s, (struct sockaddr *) (&sa), sizeof(sa)) < SUCCESS) {
        std::cerr << SYS_ERROR << CONNECT_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    return(s);
}


int run_server(int port) {
    char buf[BUFFLEN] = {'\0'};

    // establish a socket and listen
    int s = establish(port);

    // accept connection

    struct sockaddr_in isa;
    int i = sizeof(isa);
    // find socket's address
    getsockname(s, (struct sockaddr *) &isa, (socklen_t *) &i);
    // accept connection if there is one */
    int c = accept(s,  (struct sockaddr *) (&isa), (socklen_t *) (&i));
    if (c < SUCCESS) {
        std::cerr << SYS_ERROR << ACCEPT_ERROR << std::endl;
        exit(EXIT_FAIL);
    }

    //read terminal command from connection
    read_data(c, buf, BUFFLEN - 1);

    // execute terminal command
    if (system(buf) != 0) {
        std::cerr << SYS_ERROR << INVALID_COMMAND << std::endl;
        exit(EXIT_FAIL);
    }

    close(c);
    close(s);
    return 0;
}

int run_client(int port, char* terminal_command_to_run) {
    char myname[MAXHOSTNAME+1];

    // connect to socket on self
    // TODO make sure not given
    gethostname(myname, MAXHOSTNAME);
    int s = call_socket(myname, port);

    // write terminal command into socket
    write_data(s, terminal_command_to_run, strlen(terminal_command_to_run));

    close(s);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << SYS_ERROR << ARG_NUM_ERROR << std::endl;
        exit(EXIT_FAIL);
    }
    char* type = argv[1];
    int port = std::stoi(argv[2]);
    if (strcmp(type, "server") == SUCCESS) {
        return run_server(port);
    }
    else if (strcmp(type, "client") == SUCCESS) {
        if (argc < 4) {
            std::cerr << SYS_ERROR << ARG_NUM_ERROR << std::endl;
            exit(EXIT_FAIL);
        }
        return run_client(port, argv[3]);
    }
    else {
        std::cerr << SYS_ERROR << INVALID_ARG << std::endl;
        exit(EXIT_FAIL);
    }
}