/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

FILE *file_log; //for proxy.log filess

typedef struct connaddr_{
    int conn_;
    struct sockaddr_storage addr_;
}connaddr;
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void *thread(void *vargp);
void echo(int connfd,struct sockaddr_storage *sockaddr);

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd, connfdp; //descritors
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; //client address 
    char hostname[MAXLINE], port[MAXLINE];
    pthread_t tid;

    connaddr *Connaddr;
    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }
    Sleep(5);
    int portnum =atoi(argv[1]);
    listenfd=open_listenfd(portnum); //waiting connect socket

    file_log=fopen("proxy.log","a");

    while (1){
        clientlen=sizeof(struct sockaddr_storage); //size of client address
        Connaddr=malloc(sizeof(connaddr));
        connfdp=accept(listenfd, (SA *)&clientaddr, &clientlen); // connect socket
            printf("hello2\n");
            getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        
        printf("Accepted connection from (%s, %s)/\n",hostname, port);
        Connaddr->conn_=connfdp;
        Connaddr->addr_=clientaddr;
        pthread_create(&tid, NULL, thread, (void *)Connaddr);
    }    
    exit(0);
}

/* thread - peer threads*/
void *thread(void *vargp){
    //int connfd= *((int *)vargp);
    connaddr *Connaddr =(connaddr*)vargp;
    int connfd= Connaddr->conn_;
    struct sockaddr_storage sockaddr = Connaddr->addr_;
    pthread_detach(pthread_self()); //reap thread
    free(vargp);   
    echo(connfd, &sockaddr); //read, write service
    close(connfd);
    return NULL;
}

/* echo service */
void echo(int connfd, struct sockaddr_storage *sockaddr){
    size_t n; //long unsigned int, for while
    struct sockaddr_storage *addr = sockaddr;
    int clientfd=0; //descriptor
    char host[MAXLINE],uri[MAXLINE],buf[MAXLINE], port[MAXLINE];
    char method[MAXLINE], version[MAXLINE], path[MAXLINE];
    char log_entry[MAXLINE];

    while((n=read(connfd, buf, MAXLINE))!=0){
        char *p =buf; 
        printf("server received %d bytes\n", (int)n);
        printf("%s\n", buf);
        while(*p){// change lower character to upper character
            *p=toupper(*p);
            p++;
        }
        sscanf(buf,"%s %s %s", method, uri, version);
        parse_uri(uri, host, path, port);

        //clientfd = open_clientfd(host, port);
        //wirte(clientfd,buf,MAXLINE);
        write(connfd, buf, n);

        format_log_entry(log_entry,addr,uri,n);
        fprintf(file_log, "%s %d\n", log_entry, n);
        fflush(file_log);
    }

}

/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry ( char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}
