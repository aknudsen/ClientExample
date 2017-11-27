#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

unsigned int bufSize = 100000;
char *send_Buffer1 = 0;
int buf1ptr = 0;
char *send_Buffer2 = 0;
int buf2ptr = 0;
char *recv_Buffer1 = 0;
unsigned int buf1offset = 0;
char *recv_Buffer2 = 0;
unsigned int buf2offset = 0;
int errCount = 0;
int recv_buf_size = 10000;
int reuseaddr = 1;
socklen_t intLen = sizeof(int);

int PRINT = 0;
int SocketInit(int port);
int AcceptConnection(int sd);

int main()
{
    char user_entered_hostname[50];
    printf("Please enter the hostname where the server is running: ");
    scanf("%s", user_entered_hostname);
    getchar();
    int client_socket = ConnectToServer(user_entered_hostname, 60085);
    char message[500];
    ReadMsg(client_socket, message, sizeof(message));
    printf("%s",message);
    ReadMsg(client_socket, message, sizeof(message));
    printf("%s",message);
    int user_exit = 0;
    while (user_exit == 0) {
	printf("Please enter a message:\n>");
	fgets (message, sizeof(message), stdin);
	SendMsg(client_socket, message, sizeof(message));
	if (strncmp(message, "exit", 4) == 0) {
            break;
        }
	ReadMsg(client_socket, message, sizeof(message));
        if (strncmp(message, "exit", 4) == 0) {
            user_exit = 1;
        }
	printf("Received message: %s", message);
    }
    close(client_socket);
}

/* Function: ConnectToServer
 * 1. Connect to specified server and port
 * 2. Return socket
 */
int ConnectToServer(char* hostname, int port) {
  int sd;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0){
    perror("socket");
    return -1;
  }
  //printf("Socket created\n");

  struct sockaddr_in serv_addr;

  struct hostent *host = gethostbyname(hostname);
  //printf("Looking up host entry for %s (for port %u)\n", hostname, port);

  if (!host){
      printf("Getting host by addr\n");

      host = gethostbyaddr(hostname, strlen(hostname), AF_INET);
      if (!host){
          printf("No such host %s\n", hostname);
          return -1;
      }
  }
  //printf("Got host by name\n");

  memcpy((char *)&serv_addr.sin_addr.s_addr,host->h_addr,host->h_length); /* set address */

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((unsigned int)port);

  //printf("Connecting...\n");

  if (connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("connect");
    return -1;
  }
  //printf("Successfully connected to the peer\n");
  int ret = setsockopt(sd, IPPROTO_TCP, SO_RCVBUF, &recv_buf_size, intLen);
  setsockopt(sd, IPPROTO_TCP, SO_REUSEADDR, &reuseaddr, intLen);
  if (ret < 0) printf("Cound not change the buffer size\n");

  return(sd);
}

/* Function: SocketInit
 * 1. Create a socket, bind to local IP and 'port', and listen to the socket
 * 2. Return the socket
 */
int SocketInit(int port) {
  int sd;

  sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd < 0){
    // error
    perror("socket");
    exit(1);
  }
  struct sockaddr_in serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));

  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons((unsigned short)port);

  setsockopt(sd, IPPROTO_TCP, SO_REUSEADDR, &reuseaddr, intLen);

  if (bind(sd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr_in)) < 0){
    // error
    perror("bind");
    exit(1);
  }

  listen(sd, 10);

  return(sd);
}

/* Function: AcceptCall
 * 1. Accept a connection request
 * 2. return the accepted socket
 */
int AcceptConnection(int sockfd) {
  int newsock;
  struct sockaddr_in sa;
  socklen_t sockaddr_len;
  struct hostent *client;

  sockaddr_len = sizeof(struct sockaddr_in);
  newsock = accept(sockfd, (struct sockaddr *)&sa, &sockaddr_len);
  if (newsock < 0){
    perror("accept");
    exit(1);
  }

  client = gethostbyaddr(&sa.sin_addr, sizeof(sa.sin_addr), AF_INET);
  if (!client){
    printf("%d: ", h_errno);

    perror("gethostbyaddr");
    exit(1);
  }

  //printf("admin: join from '%s' at '%u'\n", client->h_name, ntohs(sa.sin_port));
  int ret = setsockopt(newsock, IPPROTO_TCP, SO_RCVBUF, &recv_buf_size, intLen);
  setsockopt(newsock, IPPROTO_TCP, SO_REUSEADDR, &reuseaddr, intLen);
  if (ret < 0) printf("Cound not change the buffer size\n");
  return newsock;
}

/* Function: ReadMsg
 * Read a message
 */
int ReadMsg(int fd, char* buff, int size)
{
  int nread = read(fd, buff, size);
  buff[nread] = '\0';
  return(nread);
}

/* Function: SendMsg
 * send a message
 */
int SendMsg(int fd, char* buff, int size)
{
    int ret=0;
    int i;
    if(PRINT){
        printf("Sending buffer: ");
        for(i=0;i<size;i++){
            printf("%x ", buff[i]);
        }
        printf("\n");
        printf("Sending %d bytes\n", size);
    }

    i=0;

    while(ret < size){
        //if (ret > 0) printf("Still need to send %d\n", size-ret);

        i = write(fd, buff+ret, size-ret);
        if(i<0){
            printf("Error when writing: %s\n", strerror(errno));
            exit(1);
        }
        ret += i;
    }
    return ret;
}
