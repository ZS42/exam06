#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct s_client
{
    int id;
    int open;
    char *msg;
}t_client;

int maxfd = 0;
int sockfd =0;
int clientid = 0;

fd_set readfds;
fd_set writefds;
fd_set currfds;

t_client clients[2556];// find using ulimit -n and subtract 4 for usuali/o and serversocket
char sendbuf[1024];
char recvbuf[1024];
char *msg = NULL;

void return_error()
{
    for (int fd = 3; fd <= maxfd; fd++)
    {
        if (clients[fd].msg)
            free(clients[fd].msg);
        clients[fd].msg = NULL;
        if (clients[fd].open)
            close(fd);
    }
    write(2, "Fatal error\n", 12);
    exit(1);
}
// understand the double pointers
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return_error();//changed return(-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
    // changed to below
    // if (buf != 0)
	// 	strcat(newbuf, buf);//
	// free(buf);
	// strcat(newbuf, add);
    {
		strcat(newbuf, buf);
	    free(buf);
        buf = NULL;
    }
    if (add)
	    strcat(newbuf, add);
	return (newbuf);
}

void send_to_all(int senderfd)
{
    for(int fd = 3; fd <= maxfd; fd++)
    {
        if(FD_ISSET(fd, &writefds) && fd != senderfd && fd != sockfd)
        {
            send(fd, sendbuf, strlen(sendbuf), 0);
            if (msg)
            send(fd, msg, strlen(msg), 0);
        }
    }
}

void add_client()
{
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
	int clientfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (clientfd < 0)
        return_error();
    // add to array and set
    clients[clientfd].open =1;
    clients[clientfd].id = clientid;
    FD_SET(clientfd, &currfds);

    // change global variables
    clientid++;
    if (maxfd < clientfd)
        maxfd = clientfd;

    // send message
    sprintf(sendbuf, "server: client %d just arrived\n", clients[clientfd].id);
    send_to_all(clientfd);
}
void rm_client(int fd)
{
    // free, close, clear, sendmsg
    if(clients[fd].msg)
    {
        free(clients[fd].msg);
        clients[fd].msg = NULL;
    }
    clients[fd].open = 0;
    close(fd);
    FD_CLR(fd, &writefds);// not needed
    FD_CLR(fd, &currfds);
    // dont need FD_CLR for readfds bc reset as currfds in while loop in main
    sprintf(sendbuf, "server: client %d just left\n", clients[fd].id);
    send_to_all(fd);
}
void send_message(int bytes, int fd)
{
    recvbuf[bytes] = '\0';
    clients[fd].msg = strjoin(clients[fd].msg, recvbuf);
    while(extract_message(&(clients[fd].msg), &msg))
    {
        sprintf(sendbuf, "client %d: ", clients[fd].id);
        send_to_all(fd);
        if(msg)
        {
            free(msg);
            msg = NULL;
        }
    }
}
int main(int ac, char **av)
{//change first 2 lines to upto struct
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(10);
    }
    int port = atoi(av[1]);

	struct sockaddr_in servaddr; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		return_error();
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		return_error();
	} 
	if (listen(sockfd, 10) != 0) {
		return_error();
	}
	// init all fd_sets
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_ZERO(&currfds);

    // add sockfd to currfds
    FD_SET(sockfd, &currfds);

    // init maxfd
    maxfd = sockfd;
    while(1)
    {
        readfds = currfds;
        writefds = currfds;
        if(select(maxfd + 1, &readfds, &writefds, NULL, NULL) <= 0)
            continue; //keep server going even if nothing is read or error
        for(int fd = 3; fd <= maxfd; fd++)
        {
            if(FD_ISSET(fd, &readfds))
            {
                if (fd == sockfd)
                    add_client();
                else
                {
                    int bytes = recv(fd, recvbuf,1023, 0);
                    if (bytes <= 0)//if bytes == 0 client closed connection if < 0 means error and should close connection
                        remove_client(fd);
                    else
                    send_message(bytes, fd);
                }
            }
        }
    }
    return(0);
}