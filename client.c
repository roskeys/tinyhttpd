#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <strings.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        printf("[ERROR] Not enough parameters");
        exit(1);
    }
    u_short port = 8080;
    char *host = "127.0.0.1";
    if (2 == argc)
    {
        port = strtol(argv[1], NULL, 10);
    }
    if (3 == argc)
    {
        host = (char*)argv[1];
        port = strtol(argv[2], NULL, 10);
    }

    int sock = -1;
    struct sockaddr_in server;
    socklen_t server_len = sizeof(server);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(host);
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("[ERROR] Create sock failed");
        exit(2);
    }
    if (connect(sock, (struct sockaddr *)&server, server_len) < 0)
    {
        printf("[ERROR] Connection failed");
        exit(3);
    }
    printf("[INFO] Connected:\n");
    char message[1024];
    char buf[1024];
    sprintf(message, "POST /date.cgi HTTP/1.1\r\n"
                     "Host: %s:%d\r\n"
                     "Upgrade-Insecure-Requests: 1\r\n"
                     "Content-Length: 10"
                     "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/95.0.4638.54 Safari/537.36\r\n"
                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9\r\n"
                     "Accept-Encoding: gzip, deflate\r\n"
                     "Accept-Language: zh-CN,zh;q=0.9\r\n"
                     "Connection: close\r\n\r\n"
                     "color=gray",
            host, port);
    send(sock, message, strlen(message), 0);
    int n;
    while ((n = read(sock, &buf, sizeof(buf))) > 0){
        printf("%s", buf);
    }
    close(sock);
    return 0;
}

// GET / HTTP/1.1
// Host: 10.0.1.136:1024
// Connection: keep-alive
// Pragma: no-cache
// Cache-Control: no-cache
// DNT: 1
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36 Edg/96.0.1054.62
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9
// Accept-Encoding: gzip, deflate
// Accept-Language: en,en-US;q=0.9,zh-CN;q=0.8,zh;q=0.7