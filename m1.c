#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct s_clients{
	int id;
	char msg[1024];
	// int still_reading;
} t_clients;

int sockfd, connfd;
socklen_t len;
struct sockaddr_in servaddr, cli;

t_clients clients[4000];// increase clients to 4000
fd_set readfds, writefds, activefds;
char readbuffer[200000], writebuffer[200000];
int fdmax, idnext;
void ft_error()
{
	write (2, "Fatal error\n", 12);
	exit (1);
}

void ft_broadcast(int ownfd)
{
	for (int fd = 0; fd <= fdmax; fd++)
	{
		if (FD_ISSET(fd, &writefds) && fd != ownfd)
		{
			send(fd, &writebuffer, strlen(writebuffer), 0);
			// size_t bytessent = send(fd, &writebuffer, strlen(writebuffer), 0);
	
			// while(bytessent != strlen(writebuffer))
			// {
			// 	size_t newsent = bytessent;
			// 	size_t bytessent = send(fd, &writebuffer[newsent], strlen(writebuffer) - newsent, 0);
			// 	bytessent+= newsent; 
			// }
		}
	}
	bzero(writebuffer, sizeof(writebuffer));
}


int main(int ac, char *av[]) {
	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit (1);
	}
	// initialise all values
	sockfd = -1;
	connfd = -1;
	bzero(&servaddr, sizeof(servaddr));
	bzero(&cli, sizeof(cli));
	bzero(&readbuffer, sizeof(readbuffer));
	bzero(&writebuffer, sizeof(writebuffer));
	bzero(&clients, sizeof(clients));
	len = sizeof(cli);
	fdmax = 0;
	idnext = 0;


	// socket create and verification dont forget to change error mangement
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		ft_error();
	} 
	
	
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); // port is provided on command line
  
	// Binding newly created socket to given IP and verification **  close fd change error management
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) {
		close (sockfd);
		ft_error();
	} 
	
	if (listen(sockfd, 10) != 0) {
		close (sockfd);
		ft_error() ;
	}
	//initialise fd_set and add sockfd
	FD_ZERO(&activefds);
	FD_SET(sockfd, &activefds);
	// change fdmax to highest fd
	fdmax = sockfd;

	while (1)
	{
		readfds = writefds = activefds;

		if (select(fdmax + 1, &readfds, &writefds, 0,0) <= 0) // dont forget to put <=
			continue;
		for (int fdI = 0; fdI <=fdmax; fdI++)
		{
			if (FD_ISSET(fdI, &readfds) && fdI == sockfd)
			{
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0) // if any error continue dont exit... client can be lazy
				continue;

				FD_SET(connfd, &activefds); // add to active fds
				clients[connfd].id = idnext; // init id of client and update nextid
				idnext++;
				if (fdmax < connfd)
					fdmax = connfd; // fdmax is always highest fd
				sprintf(writebuffer,"server: client %d just arrived\n", clients[connfd].id);
				ft_broadcast (connfd);
				break;
			}
			if (FD_ISSET(fdI, &readfds) && fdI != sockfd)
			{
				bzero(readbuffer, sizeof(readbuffer));
				int bytesrecv = recv(fdI, readbuffer, sizeof(readbuffer), 0);
				
				if (bytesrecv <= 0)
				{
					sprintf(writebuffer, "server: client %d just left\n", clients[fdI].id);
					ft_broadcast(fdI);
					FD_CLR(fdI,&activefds);
					
					close(fdI);
				}
				else{
					for (int i = 0, j = strlen(clients[fdI].msg); i < bytesrecv; i++, j++)
					{
						clients[fdI].msg[j] = readbuffer[i];
					
						if (clients[fdI].msg[j] == '\n')
						{
							clients[fdI].msg[j] = '\0';
							sprintf(writebuffer, "client %d: %s\n", clients[fdI].id, clients[fdI].msg);
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