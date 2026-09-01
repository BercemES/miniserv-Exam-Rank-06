#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct clients
{
	int		fd;
	int		id;
	char	*in;
	char	*out;
}t_cli;

t_cli	clients[1024];

int		g_next_id = 0;
int		g_max_fd = 0;

fd_set	read_fds, write_fds;

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
	if (!newbuf)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	fatal_error(void)
{
	write(2, "Fatal error\n", strlen("Fatal error\n"));
	exit (1);
}

void	broadcast(char *msg, int except_fd)
{
	int i = 0;
	while (i < 1024)
	{
		if (clients[i].fd != -1 && clients[i].fd != except_fd)
			clients[i].out = str_join(clients[i].out, msg);
		i++;
	}
}

void	accept_new_client(int sockfd)
{
	int		new_fd = 0;
	char	msg[200];

	new_fd = accept(sockfd, NULL, NULL);
	if (new_fd == -1)
		return ;
	if (new_fd >= 1024)
    {
        close(new_fd);
        return ;
    }
	clients[new_fd].fd = new_fd;
	clients[new_fd].id = g_next_id++;
	clients[new_fd].in = NULL;
	clients[new_fd].out = NULL;
	sprintf(msg, "server: client %d just arrived\n", clients[new_fd].id);
	broadcast(msg, new_fd);
}

void	remove_clients(int fd)
{
	char	msg[100];

	sprintf(msg, "server: client %d just left\n", clients[fd].id);
	broadcast(msg, fd);
	close(fd);
	clients[fd].fd = -1;
	free(clients[fd].in);
	free(clients[fd].out);
	clients[fd].in = NULL;
	clients[fd].out = NULL;
}

void	read_clients(int fd)
{
	int		res;
	char	read_buffer[1000];

	res = recv(fd, read_buffer, 999, 0);
	if (res <= 0)
	{
		remove_clients(fd);
		return ;
	}
	read_buffer[res] = '\0';
	clients[fd].in = str_join(clients[fd].in, read_buffer);

	int		ret = 0;
	char	*line = NULL;

	while ((ret = extract_message(&clients[fd].in, &line)) == 1)
	{
		char	*msg;
		msg = malloc(strlen(line) + 100);
		if (!msg)
			fatal_error();
		sprintf(msg, "client %d: %s", clients[fd].id, line);
		broadcast(msg, clients[fd].fd);
		free(line);
		free(msg);
	}
	if (ret == -1)
		fatal_error();
}

void	write_clients(int fd)
{
	int	res = 0;

	res = send(fd, clients[fd].out, strlen(clients[fd].out), 0);
	if (res <= 0)
	{
		remove_clients(fd);
		return ;
	}
	if (res == (int)strlen(clients[fd].out))
	{
		free(clients[fd].out);
		clients[fd].out = NULL;
	}
	else
	{
		char	*newbuf;
		newbuf = malloc(strlen(clients[fd].out + res) + 1);
		if (!newbuf)
			fatal_error();
		newbuf = strcpy(newbuf, clients[fd].out + res);
		if (!newbuf)
			fatal_error();
		free(clients[fd].out);
		clients[fd].out = newbuf;
	}
}

void	handle_clients(int sockfd)
{
	for (int i = 0; i < 1024; i++)
		clients[i].fd = -1;
	while (1)
	{
		FD_ZERO(&read_fds);
		FD_ZERO(&write_fds);
		FD_SET(sockfd, &read_fds);
		g_max_fd = sockfd;
		for (int i = 0; i < 1024; i++)
		{
			if (clients[i].fd == -1)
				continue ;
			FD_SET(clients[i].fd, &read_fds);
			if (clients[i].out != NULL)
				FD_SET(clients[i].fd, &write_fds);
			if (g_max_fd < clients[i].fd)
				g_max_fd = clients[i].fd;
		}
		if (select(g_max_fd + 1, &read_fds, &write_fds, NULL, NULL) == -1)
			fatal_error();
		if (FD_ISSET(sockfd, &read_fds))
			accept_new_client(sockfd);
		for (int i = 0; i < 1024; i++)
		{
			if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &read_fds))
				read_clients(clients[i].fd);
			
			if (clients[i].fd != -1 && FD_ISSET(clients[i].fd, &write_fds))
				write_clients(clients[i].fd);
		}
	}
}

int main(int ac, char **av) {
	int sockfd;
	struct sockaddr_in servaddr; 

	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", strlen("Wrong number of arguments\n"));
		exit(1);
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatal_error();
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal_error();
	if (listen(sockfd, 100) != 0)
		fatal_error();
	handle_clients(sockfd);
}