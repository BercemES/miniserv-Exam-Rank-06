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
    int   id;
    char  *msg_to_read;
    char  *pending_output;
} t_cli;

t_cli   g_clients[65536];
fd_set  active_fds, read_fds, write_fds;
int     g_max_fd = 0;
int     g_next_id = 0;
int     g_sockfd = 0;

char    buf_read[100100];
char    buf_send[100100];

void fatal_error(void)
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

int extract_message(char **buf, char **msg)
{
    char *newbuf;
    int  i = 0;

    *msg = 0;
    if (*buf == 0)
        return (0);
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
    char *newbuf;
    int  len = (buf == 0) ? 0 : strlen(buf);

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

void queue_msg(int fd, char *msg)
{
    g_clients[fd].pending_output = str_join(g_clients[fd].pending_output, msg);
    if (!g_clients[fd].pending_output)
        fatal_error();
}

void broadcast(int sender_fd, char *msg)
{
    int len = strlen(msg);

    for (int fd = 0; fd <= g_max_fd; fd++)
    {
        if (fd == sender_fd || fd == g_sockfd || g_clients[fd].id == -1)
            continue;
        if (g_clients[fd].pending_output != NULL)
        {
            queue_msg(fd, msg);
            continue;
        }
        int sent = send(fd, msg, len, 0);
        if (sent < 0)
            queue_msg(fd, msg);
        else if (sent < len)
            queue_msg(fd, msg + sent);
    }
}

void accept_new_client(int sockfd)
{
    int connfd = accept(sockfd, NULL, NULL);
    if (connfd < 0)
        return;
    if (connfd > g_max_fd)
        g_max_fd = connfd;
    g_clients[connfd].id = g_next_id++;
    g_clients[connfd].msg_to_read = NULL;
    g_clients[connfd].pending_output = NULL;
    FD_SET(connfd, &active_fds);
    sprintf(buf_send, "server: client %d just arrived\n", g_clients[connfd].id);
    broadcast(connfd, buf_send);
}

void remove_client(int fd)
{
    sprintf(buf_send, "server: client %d just left\n", g_clients[fd].id);
    broadcast(fd, buf_send);
    FD_CLR(fd, &active_fds);
    close(fd);
    if (g_clients[fd].msg_to_read)
    {
        free(g_clients[fd].msg_to_read);
        g_clients[fd].msg_to_read = NULL;
    }
    if (g_clients[fd].pending_output)
    {
        free(g_clients[fd].pending_output);
        g_clients[fd].pending_output = NULL;
    }
    g_clients[fd].id = -1;
}

void read_client(int fd)
{
    int read_bytes = recv(fd, buf_read, 100000, 0);

    if (read_bytes <= 0)
        remove_client(fd);
    else
    {
        buf_read[read_bytes] = '\0';
        g_clients[fd].msg_to_read = str_join(g_clients[fd].msg_to_read, buf_read);
        if (!g_clients[fd].msg_to_read)
            fatal_error();

        char *msg = NULL;
        int ret;
        while ((ret = extract_message(&g_clients[fd].msg_to_read, &msg)) == 1)
        {
            sprintf(buf_send, "client %d: %s", g_clients[fd].id, msg);
            broadcast(fd, buf_send);
            free(msg);
            msg = NULL;
        }
        if (ret == -1)
            fatal_error();
    }
}

void write_client(int fd)
{
    if (!g_clients[fd].pending_output)
        return;
    int len = strlen(g_clients[fd].pending_output);
    int sent = send(fd, g_clients[fd].pending_output, len, 0);
    if (sent > 0)
    {
        if (sent == len)
        {
            free(g_clients[fd].pending_output);
            g_clients[fd].pending_output = NULL;
        }
        else
        {
            char *new_out = malloc(len - sent + 1);
            if (!new_out)
                fatal_error();
            strcpy(new_out, g_clients[fd].pending_output + sent);
            free(g_clients[fd].pending_output);
            g_clients[fd].pending_output = new_out;
        }
    }
}

void handle_clients(int sockfd)
{
    for (int i = 0; i < 65536; i++)
        g_clients[i].id = -1;

    while (1)
    {
        read_fds = write_fds = active_fds;

        if (select(g_max_fd + 1, &read_fds, &write_fds, NULL, NULL) < 0)
            continue;

        for (int fd = 0; fd <= g_max_fd; fd++)
        {
            if (FD_ISSET(fd, &read_fds))
            {
                if (fd == sockfd)
                    accept_new_client(sockfd);
                else
                    read_client(fd);
            }
            if (fd != sockfd && FD_ISSET(fd, &write_fds))
            {
                write_client(fd);
            }
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

    g_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (g_sockfd < 0)
        fatal_error();

    g_max_fd = g_sockfd;
    FD_ZERO(&active_fds);
    FD_SET(g_sockfd, &active_fds);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(av[1]));

    if (bind(g_sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        fatal_error();

    if (listen(g_sockfd, 10) < 0)
        fatal_error();

    handle_clients(g_sockfd);
    return (0);
}