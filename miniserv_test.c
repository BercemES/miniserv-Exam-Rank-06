#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>


typedef struct g_clients
{
    int     fd;
    int     id;
    char    *in;
    char    *out;
}t_cli;

t_cli   clients[1024];

fd_set  read_fds, write_fds;
int     g_next_id = 0;

void fatal_error(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

int   ft_htons(int port)
{
    return (port >> 8 | port << 8);
}


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
				return (-1);
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
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void broadcast(char *msg, int except_fd)
{
    int i = 0;

    while (i < 1024)
    {
        if (clients[i].fd != -1 && clients[i].fd != except_fd)
            clients[i].out = str_join(clients[i].out, msg);
        i++;
    }
}

void    accept_new_client(int sockfd)
{
    int new_fd;
    char msg[100];

    new_fd = accept(sockfd, NULL, NULL);
    if (new_fd == -1)
        return;
    clients[new_fd].fd = new_fd;
    clients[new_fd].id = g_next_id++;
    clients[new_fd].in = NULL;
    clients[new_fd].out = NULL;
    sprintf(msg, "server: client %d just arrived\n", clients[new_fd].id);
    broadcast(msg, -1);
}

void    read_clients(t_cli client)
{
    int     res;
    char    *read_buff[1000];
    
    res = recv(client.fd, read_buff, 999, 0);
    if (res <= 0)
        remove_client();
    else
    {
        read_buff[res] = '\0';
        client.in = str_join(client.in, read_buff);
        
        char    *line = NULL;

        while(extract_message(client.in, &line) == 1)
        {
            char *msg = malloc(strlen(line) + 100) //sprintf ile int string yazabiliyoruz o yüzden client.id için temsili fazladan 50 byte ekliyoruz.
            if (!msg)
                fatal_error();
            sprintf(msg, "client %d: %s", clients[i].id, line);
            broadcast(msg, client.fd);
            free(msg);
            free(line);
        }
    }
}

void    handle_clients(int sockfd)
{
    int i = 0;
    int max_fd;

    for (i = 0; i < 1024; i++)
        clients[i].fd = -1;
    g_next_id = 0;
    while (1)
    {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_SET(sockfd, &read_fds);
        max_fd = sockfd;

        for(i = 0; i < 1024; i++)
        {
            if (clients[i].fd == -1)
                continue ;
            FD_SET(clients[i].fd, &read_fds);
            if (clients[i].out != NULL)
				FD_SET(clients[i].fd, &write_fds);
			if (clients[i].fd > max_fd)
				max_fd = clients[i].fd;
        }
        if (select(max_fd + 1, &read_fds, &write_fds, NULL, NULL) == -1)
                fatal_error();
        if (FD_ISSET(sockfd, &read_fds))
            accept_new_client(sockfd);
        for (i = 0; i < 1024; i++)
        {
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds))
                read_clients(clients[i]);// ssize_t recv(int sockfd, void buf[.len], size_t len, int flags);
            if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &write_fds))
                // ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
        }
    }
}

int main(int ac, char **av)
{
    if (ac != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit(1);
    }
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
        fatal_error();
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = 1 << 24 | 127;
    //servaddr.sin_addr.s_addr = htonl(INNADDR_ANY);
    servaddr.sin_port = ft_htons(atoi(av[1]));
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
        fatal_error();
    if (listen(sockfd, 10) != 0)
        fatal_error();
    handle_clients(sockfd);
}