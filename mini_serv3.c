#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>

typedef struct		s_client 
{
	int				fd;
    int             id;
	struct s_client	*next;
}	t_client;

t_client *g_clients = NULL;
// adding here. dont know why 65536. set to 0 if sentence ended and 1 if not
int sentence_end[65536];
int sock_fd, g_id = 0;
char temp_buff        [4096 * 42];
char read_buff        [4096 * 42];
char send_buff        [4097 * 42];
// fd_set all_fds;
// fd_set read_fds;
// fd_set write_fds;
fd_set curr_sock_fds, read_fds, write_fds;
char msg[42];
// char temp_buff[42*4096], read_buff[42*4096], send_buff[42*4096 + 42];

void	fatal() 
{
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	close(sock_fd);
	exit(1);
}

int get_id(int fd)
{
    t_client *temp = g_clients;

    while (temp)
    {
        if (temp->fd == fd)
            return (temp->id);
        temp = temp->next;
    }
    return (-1);
}

int		get_max_fd() 
{
	int	max = sock_fd;
    t_client *temp = g_clients;

    while (temp)
    {
        if (temp->fd > max)
            max = temp->fd;
        temp = temp->next;
    }
    return (max);
}

void	send_all(int fd, char *str_req)
{
    t_client *temp = g_clients;

    while (temp)
    {
        // FD_ISSET checks if the file descriptor fd is a member of the fd_set pointed to by set. Returns true (non-zero) if fd is present in set, indicating that fd has pending events (read/write ready).
        // FD_ISSET(int fd, fd_set *set):
        if (temp->fd != fd && FD_ISSET(temp->fd, &write_fds))
        {
            if (send(temp->fd, str_req, strlen(str_req), 0) < 0)
                fatal();
        }
        temp = temp->next;
    }
}

int		add_client_to_list(int fd)
{
    t_client *temp = g_clients;
    t_client *new;

    if (!(new = calloc(1, sizeof(t_client))))
        fatal();
    new->id = g_id++;
    new->fd = fd;
    new->next = NULL;
    if (!g_clients)
    {
        g_clients = new;
    }
    else
    {
        while (temp->next)
            temp = temp->next;
        temp->next = new;
    }
    return (new->id);
}

void add_client()
{
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    int client_fd;

    if ((client_fd = accept(sock_fd, (struct sockaddr *)&clientaddr, &len)) < 0)
        fatal();
    sprintf(msg, "server: client %d just arrived\n", add_client_to_list(client_fd));
    send_all(client_fd, msg);
    // Adds the file descriptor fd to the fd_set pointed to by set. After calling FD_SET(fd, set), fd will be monitored for read/write readiness when () is called.
    FD_SET(client_fd, &curr_sock_fds);
}

int rm_client(int fd)
{
    t_client *temp = g_clients;
    t_client *del;
    int id = get_id(fd);

    if (temp && temp->fd == fd)
    {
        g_clients = temp->next;
        free(temp);
    }
    else
    {
        while(temp && temp->next && temp->next->fd != fd)
            temp = temp->next;
        del = temp->next;
        temp->next = temp->next->next;
        free(del);
    }
    return (id);
}

// void ex_msg(int fd)
// {
//     int i = 0;
//     int j = 0;

//     while (str[i])
//     {
//         tmp[j] = str[i];
//         j++;
//         if (str[i] == '\0')
//         {
//             sentence_end[fd] = 1;
//         }
//         if (str[i] == '\n')
//         {
//             // adding here
//             str[i + 1] = '\0';
//             sprintf(buf, "client %d: %s", get_id(fd), tmp);
//             send_all(fd, buf);
//             j = 0;
//             bzero(&tmp, strlen(tmp));
//             bzero(&buf, strlen(buf));
//             sentence_end[fd] = 0;
//         }
//         i++;
//     }
//     bzero(&str, strlen(str));
// }
void flush_send_buff(int sender_fd) //send send_buff to everyone except sender
{
    int max_fd = get_max_fd();
    for(int fd = 0; fd <= max_fd; fd++)
    {
        if(FD_ISSET(fd, &write_fds) && sender_fd != fd)
            send(fd, send_buff, strlen(send_buff), 0);
    }
}

void flush_temp_buff(int sender_fd) //send temp_buff to everyone except sender and eventually adds "client %d: "
{
    if (sentence_end[sender_fd])
        sprintf(send_buff, "%s", temp_buff);
    else
        sprintf(send_buff, "client %d: %s", get_id(sender_fd), temp_buff);
    flush_send_buff(sender_fd);
}
int main(int ac, char **av)
{
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
        exit(1);
    }

    struct sockaddr_in servaddr;
    uint16_t port = atoi(av[1]);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port);

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        fatal();
    if (bind(sock_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        fatal();
        // changing from 0 to 128 didnt make a difference
    if (listen(sock_fd, 128) < 0)
        fatal();
    // Initializes the fd_set pointed to by set to be empty, i.e., containing no file descriptors.
    FD_ZERO(&curr_sock_fds);
    // Adds the file descriptor fd to the fd_set pointed to by set. After calling FD_SET(fd, set), fd will be monitored for read/write readiness when select() is called.
    FD_SET(sock_fd, &curr_sock_fds);
    bzero(&temp_buff, sizeof(temp_buff));
    bzero(&read_buff, sizeof(read_buff));
    bzero(&send_buff, sizeof(send_buff));
    while(1)
    {
        // write_ = read = curr_sock;
        if (select(get_max_fd() + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= get_max_fd(); fd++)
        {
            if (FD_ISSET(fd, &read_fds))
            {
                if (fd == sock_fd)
                {
                    bzero(&msg, sizeof(msg));
                    add_client();
                    break;
                }
                else
                {
			int ret_recv = 1000;
			while (ret_recv == 1000 || read_buff[strlen(read_buff) - 1] != '\n')
			{
                // why 1000 or why 4096*42 for length of data buffer
                // 0 is flag
                // pointer to data buffer is str + strlen(str) or read_buff why? 
				// ret_recv = recv(fd, str + strlen(str), 1000, 0);
                ret_recv = recv(fd, read_buff, 1000, 0);
				if (ret_recv <= 0)
					break ;
			}
                    if (ret_recv <= 0)
                    {
                        bzero(&msg, sizeof(msg));
                        sprintf(msg, "server: client %d just left\n", rm_client(fd));
                        send_all(fd, msg);
                        FD_CLR(fd, &curr_sock_fds);
                        close(fd);
                        break;
                    }
                    else
                    {
                        read_buff[ret_recv] =    '\0';                                //make read_buff null terminated to iterate over it easily
                        temp_buff[0] =            '\0';                                //reset temp_buff

                        char *ret = read_buff;                                        //reticule on read buffer. Will move from sentences to sentences
                        while (ret[0] != '\0')
                        {
                            strcpy(temp_buff, ret);                                    //we work on a copy of read_buff, at position ret

                            int i = 0;
                            while (temp_buff[i] != '\0' && temp_buff[i] != '\n')    //i will stop on a \n or \0
                                i++;

                            if (temp_buff[i] == '\0')                                //partial end, no more to read
                            {
                                flush_temp_buff(fd);
                                sentence_end[fd] = 1;                                //not the end of a sentence
                                break;
                            }
                            if (temp_buff[i] == '\n')                                //end of sentence, but maybe there is still more to read
                            {
                                temp_buff[i + 1] = '\0';                            //close temp_buff
                                ret += i + 1;                                        //ret jump to begining of new sentence
                                flush_temp_buff(fd);
                                sentence_end[fd] = 0;                                //end of sentence
                            }
                        }
                            // ex_msg(fd);
                    }
                }
            }
            
        }
        
    }
    return (0);
}