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
#define SERVER_STRING "Server: myhttpd/0.1.0\r\n"

void response_code(int, const char *);
void headers(int, const char *);
void read_file(int, FILE *);
void not_found(int, const char *);
void unimplemented(int);
void serve_file(int, const char *);
void handle_connection(void *);
int get_line(int, char *, int);

void response_code(int client, const char *status)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "HTTP/1.0 %s\r\n", status);
    send(client, buf, strlen(buf), 0);
}

void headers(int client, const char *filename)
{
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    (void)filename;
    sprintf(buf, "%sContent-Type: text/html\r\n\r\n", SERVER_STRING);
    send(client, buf, strlen(buf), 0);
}

void read_file(int client, FILE *file)
{
    char buf[1024];
    memset(buf, 0, sizeof(buf));

    while (!feof(file))
    {
        fgets(buf, sizeof(buf), file);
        if (strlen(buf) > 0)
        {
            send(client, buf, strlen(buf), 0);
        }
    }
}

void not_found(int client, const char *path)
{
    printf("[INFO] File not found %s\n", path);
    response_code(client, "404 NOT FOUND");
    headers(client, path);
    FILE *file_404 = fopen("www/404.html", "r");
    if (file_404 != NULL)
    {
        read_file(client, file_404);
    }
}

void unimplemented(int client)
{
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "HTTP/1.0 501 Method Not Implemented\r\n"
                 "Server: %s"
                 "Content-Type: text/html\r\n\r\n"
                 "<HTML>"
                 "<HEAD><TITLE>Method Not Implemented</TITLE></HEAD>"
                 "<BODY><h1>HTTP request method not supported.</h1></BODY>"
                 "</HTML>\r\n\r\n",
            SERVER_STRING);
    send(client, buf, strlen(buf), 0);
}

void serve_file(int client, const char *filename)
{
    FILE *file = NULL;
    file = fopen(filename, "r");
    if (file == NULL)
    {
        not_found(client, filename);
    }
    else
    {
        printf("[INFO] Serve file %s\n", filename);
        response_code(client, "200 OK");
        headers(client, filename);
        read_file(client, file);
    }
    fclose(file);
}

int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && c != '\n')
    {
        n = recv(sock, &c, 1, 0);
        if (n > 0)
        {
            buf[i] = c;
            i++;
        }
        else
        {
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
}

void handle_connection(void *arg)
{
    int client = (intptr_t)arg, n;
    struct stat st;
    size_t i, j;
    char buf[1024];
    char method[16];
    char url[500], path[524];
    memset(buf, 0, sizeof(buf));
    memset(url, 0, sizeof(url));
    memset(method, 0, sizeof(method));

    // get first line GET / HTTP/1.1
    n = get_line(client, buf, sizeof(buf));

    // read method from the buffer
    while (!isspace((int)buf[i]) && i < sizeof(method) - 1)
    {
        method[i] = buf[j];
        i++;
        j++;
    }
    method[i] = '\0';

    // check http method is GET and POST only
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST"))
    {
        unimplemented(client);
        return;
    }

    // remove the space after method
    while (isspace((int)buf[j]) && j < sizeof(buf))
    {
        j++;
    }

    // read the url from the buffer
    i = 0;
    while (!isspace((int)buf[j]) && i < sizeof(url) - 1 && j < sizeof(buf))
    {
        url[i] = buf[j];
        i++;
        j++;
    }
    url[i] = '\0';
    if (strcmp(url, "/") == 0 && strlen(url) == 1)
    {
        sprintf(path, "www/index.html");
    }
    else
    {
        sprintf(path, "www%s", url);
    }

    while (n > 0 && strcmp("\r\n", buf))
    {
        n = get_line(client, buf, sizeof(buf));
    }

    printf("[INFO] Path:%s\n", path);
    if (stat(path, &st) == -1)
    {
        not_found(client, path);
    }
    else
    {
        serve_file(client, path);
    }
    close(client);
}

int main(int argc, char const *argv[])
{
    pthread_t thread;
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
    int sockfd, client = -1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t server_addr_len = sizeof(server_addr);
    socklen_t client_addr_len = sizeof(client_addr);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        printf("[ERROR] Create socket failed\n");
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, server_addr_len) != 0)
    {
        printf("[ERROR] Socket bind to port %s:%d failed\n", host, port);
        exit(2);
    }
    printf("[INFO] Server listening on port %d\n", port);
    if (listen(sockfd, 5) < 0)
    {
        printf("[ERROR] Listen failed\n");
        exit(3);
    }

    while (1)
    {
        if ((client = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            printf("[ERROR] Accept client connection failed");
            exit(4);
        }
        if (pthread_create(&thread, NULL, (void *)handle_connection, (void *)(intptr_t)client) == -1)
        {
            printf("[ERROR] Creating new thread failed");
            exit(5);
        }
    }
    printf("[INFO] Exit...");
    close(sockfd);
    return 0;
}
