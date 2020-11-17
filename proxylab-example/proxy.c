/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     2014430024 - Oh JeongKyu 
 *     2014430021 - Shim YongHeon
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"


FILE *file_log; //for proxy.log filess

typedef struct connaddr_{
    int connf;
    struct sockaddr_in addre;
}connaddr;
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void *thread(void *vargp);
void proxy(int connfd,struct sockaddr_in *sockaddr);
void read_requesthdrs(rio_t *rp);

void sigpipe_handler(int sig) {
    printf("SIGPIPE handled\n");
    return;
}

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv){
    int listenfd, connfdp, port; //descriptors
    socklen_t clientlen;
    struct sockaddr_in clientaddr; //client address 
    char hostname[MAXLINE];
    pthread_t tid;
    connaddr *Connaddr;
    /* Check arguments */
    if (argc != 2) {
	fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	exit(0);
    }

    Signal(SIGPIPE, sigpipe_handler);
    int portnum =atoi(argv[1]);
    listenfd=open_listenfd(portnum); //waiting connect socket

    file_log=fopen("proxy.log","a");

    while (1){
        clientlen=sizeof(struct sockaddr_in); //size of client address
        Connaddr=malloc(sizeof(connaddr));
        connfdp=accept(listenfd, (SA *)&clientaddr, &clientlen); // connect socket
        Connaddr->connf=connfdp;// descriptor
        Connaddr->addre=clientaddr; // address
        pthread_create(&tid, NULL, thread, (void *)Connaddr); //create thread
    }    
    exit(0);
}

/* thread - peer threads*/
void *thread(void *vargp){
    //int connfd= *((int *)vargp);
    connaddr *Connaddr =(connaddr*)vargp;
    int connfd= Connaddr->connf;
    struct sockaddr_in sockaddr = Connaddr->addre;
    pthread_detach(pthread_self()); //reap thread
    free(vargp);
    Signal(SIGPIPE, sigpipe_handler);
    proxy(connfd, &sockaddr); //read, write service
    close(connfd);
    return NULL;
}

/* proxy service */
void proxy(int connfd, struct sockaddr_in *sockaddr){
    char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\nAccept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\nAccept-Encoding: gzip, deflate\r\nConnection: close\r\nProxy-Connection: close\r\n\r\n";
    size_t n; //long unsigned int, for while
    size_t sum=0;
    int clientfd; //descriptor
    int port;
    char host[MAXLINE],uri[MAXLINE],buf[MAXLINE], buf2[MAXLINE], payload[MAXLINE];
    char method[MAXLINE], version[MAXLINE], path[MAXLINE];
    char log_entry[MAXLINE];
    rio_t rio, rio_client;

    /*read prart*/
    Rio_readinitb(&rio, connfd);
    Rio_readlineb(&rio ,buf,MAXLINE);
    printf("Request headers:\n");
    printf("%s\n", buf);

    printf("----------------\n");
    
    if (strcmp(buf, "") == 0) //buf check. if buf==NULL, return.
        return;

    sscanf(buf,"%s %s %s", method, uri, version);

    read_requesthdrs(&rio);


    parse_uri(uri, host, path, &port);

    //print information 
    printf("----------* information *-----------\n");
    printf("uri = \"%s\"\n", uri);
    printf("host = \"%s\", ", host);
    printf("port = \"%d\", ", port);
    printf("path = \"%s\"\n", path);
    printf("---------* information end*---------\n");
    

    clientfd = open_clientfd(host, port);
    Rio_readinitb(&rio_client, clientfd);

    sprintf(buf2, "GET %s HTTP/1.0\r\n", path);
    write(clientfd, buf2, strlen(buf2));
	sprintf(buf2, "Host: %s\r\n", host);
    rio_writen(clientfd, buf2, strlen(buf2));
    write(clientfd, user_agent_hdr, strlen(user_agent_hdr));


    strcpy(payload, "");
    while ((n = rio_readlineb(&rio_client, buf2, MAXLINE)) != 0) {    
        sum += n;
        if (sum <= MAXLINE)
            strcat(payload, buf2);
		write(connfd, buf2, n);
	}

    // Call function that stores log
    format_log_entry(log_entry,sockaddr,uri,n);
    fprintf(file_log, "%s %d\n", log_entry, n);
    fflush(file_log);

    close(clientfd);// block the memory leak
}

/*
 * read_requesthdrs(rp)
 *  
 * Read the header and ignore the connection request.
 * 
 * Read the string in the 'buf' up to "\r\n".
 */

void read_requesthdrs(rio_t *rp)
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    while (strcmp(buf, "\r\n"))
    {
        Rio_readlineb(rp,buf,MAXLINE);
        printf("%s", buf);

    }
    return;
    
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
	//pathbegin++;	
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
