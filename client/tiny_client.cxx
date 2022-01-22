#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int sockfd = -1;

    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( -1 == sockfd ) {
        perror( "sock created" );
        exit( -1 );
    }

    struct sockaddr_in server;
    memset( &server, 0, sizeof( struct sockaddr_in ) );
    server.sin_family = AF_INET;
    server.sin_port = htons(6543);
    server.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    int res = -1;
    res = connect( sockfd, (struct sockaddr*)&server, sizeof( server ) );
    if( -1 == res ){
        perror( "sock connect" );
        exit( -1 );
    }

    char sendBuf[1024] = { 0 };
    char recvBuf[1024] = { 0 };

    char in[1024] = {0};
    while( fgets( in, sizeof( in ), stdin ) != NULL ) {
        sendBuf[0] = 0x3f;
    
        unsigned short temp = 8 + strlen("nihao");
        memcpy(sendBuf + 1, &temp, sizeof(temp));

        temp = 0;
        memcpy(sendBuf + 3, &temp, sizeof(temp));

        temp = 0;
        memcpy(sendBuf + 5, &temp, sizeof(temp));

        memcpy(sendBuf + 9, "nihao", strlen("nihao"));

        int total = 9 + strlen("nihao");

        write( sockfd, sendBuf, total );
        read( sockfd, recvBuf, sizeof( recvBuf ) );

        if(recvBuf[0] == 0x3f)
        {
            int rptLen = 0;
            char sstr[100] = {0};
            memcpy(&rptLen, recvBuf + 1, 2);
            memcpy(sstr, recvBuf + 9, rptLen - 8);
            fputs( sstr, stdout );
        }

        memset( in, 0, sizeof( in ) );
        memset( recvBuf, 0, sizeof( recvBuf ) );
        memset( sendBuf, 0, sizeof( sendBuf ) );
    }

    close( sockfd );

    return 0;
}