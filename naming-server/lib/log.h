#ifndef LOG_H
#define LOG_H

void get_ip_address(int client_socket, char *ip_addr);
void bookkeep(int client_socket, int flag, const char *type, const char *data);
void init_bookkeep();
void log_client_request(int socket_fd, ClientMessage *request);

#endif