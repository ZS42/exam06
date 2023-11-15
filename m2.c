#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct s_clients
{
	int id;
	char msg[1024];
} t_clients;

int sockfd, connfd;
socklen_t len;
struct sockaddr_in servaddr, cli; 
t_clients clients[4000];
fd_set readfds, writefds, activefds;
char readbuff[200000], writebuff[200000];
int fdmax, nextid;

void ft_error()
{
	write (2, "Fatal error\n", 12);
	exit (1);
}

void ft_broadcast(int ownfd)
{
	for (int fd = 0; fd <= fdmax; fd++)
	{
		if (FD_ISSET(fd , &writefds) && fd != ownfd)
		{
			size_t sentbytes = send (fd, &writebuff, strlen(writebuff), 0);
			while (sentbytes != strlen(writebuff))
			{
				size_t newsent = sentbytes;
				sentbytes = send (fd, &writebuff[newsent], strlen(writebuff) - newsent, 0);
				sentbytes += newsent;
			}
		}
	}
}

int main(int ac, char *av[]) {
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit (1);
	}

	sockfd = -1;
	connfd = -1;

bzero(&servaddr, sizeof(servaddr));
bzero(&cli, sizeof(cli));
len = sizeof(cli);
bzero(clients, sizeof(clients));
bzero(readbuff, sizeof(readbuff));
bzero( writebuff, sizeof( writebuff));

fdmax = 0;
nextid = 0;
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		ft_error() ;
	} 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		close (sockfd);
		ft_error() ;
	} 
	
	if (listen(sockfd, 4000) != 0) {
		close (sockfd);
		ft_error() ;
	}
	FD_ZERO(&activefds);
	FD_SET(sockfd, &activefds);
	fdmax = sockfd;

	while (1)
	{
		readfds = writefds = activefds;

		if (select(fdmax + 1, &readfds, &writefds, 0,0) <= 0)
			continue;
		for (int fdI = 0; fdI <= fdmax; fdI++)
		{
			if (FD_ISSET(fdI, &readfds) && fdI == sockfd)
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0)
					continue;
				FD_SET(connfd, &activefds);
				clients[connfd].id = nextid;
				nextid++;
				 if (fdmax < connfd)
				 	fdmax = connfd;
				sprintf(writebuff, "server: client %d just arrived\n", clients[connfd].id);
				ft_broadcast(connfd);
				break;
			}
			if (FD_ISSET(fdI, &readfds) && fdI != sockfd)
			{
				bzero(&readbuff, sizeof(readbuff));
				int bytesrecv = recv(fdI, &readbuff, sizeof(readbuff), 0);

				if (bytesrecv <= 0)
				{
					sprintf(writebuff, "server: client %d just left\n", clients[fdI].id);
					ft_broadcast(fdI);
					FD_CLR(fdI, &activefds);
					close(fdI);
				}
				else
				{
					for (int i = 0, j = strlen(clients[fdI].msg); i < bytesrecv; i++, j++)
					{
						clients[fdI].msg[j] = readbuff[i];
						if (clients[fdI].msg[j] == '\n')
						{
							clients[fdI].msg[j] = '\0';
							sprintf(writebuff, "client %d: %s\n", clients[fdI].id,clients[fdI].msg);
							ft_broadcast(fdI);
							bzero(clients[fdI].msg, sizeof(clients[fdI].msg));
							j = -1;
						}
					}
				}
				break;
			}
		}
	}
	
}