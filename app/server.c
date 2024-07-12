#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define MAX_RESPONSE_SIZE 1024

char *get_response(char *buffer);

int main() {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  // Declare variables for the server file descriptor and client address
  // information.
  int server_fd, client_addr_len;
  struct sockaddr_in client_addr;

  /*
  The socket() function is called to create a new socket. It uses IPv4
  (AF_INET is the address family) and TCP (SOCK_STREAM is the type of
  socket) and returns a file descriptor for the new socket. If the
  socket() function fails, it returns -1 and sets errno to indicate the
  error.
  */
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    printf("Socket creation failed: %s...\n", strerror(errno));
    return 1;
  }

  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors. This
  // allows the program to reuse the same address quickly if it's restarted.
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) <
      0) {
    printf("SO_REUSEADDR failed: %s \n", strerror(errno));
    return 1;
  }
  /* It sets up a sockaddr_in structure (serv_addr) with the server's address
    information:
    * AF_INET for IPv4
    * Port 4221
    * INADDR_ANY to listen on all available network interfaces
    */
  struct sockaddr_in serv_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(4221),
      .sin_addr = {htonl(INADDR_ANY)}, // Set server addres to 0.0.0.0
  };

  // The bind() function is called to associate the socket with the server
  // address and port.
  if (bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
    printf("Bind failed: %s \n", strerror(errno));
    return 1;
  }

  /* The listen() function is called to make the socket passive and ready to
   * accept incoming connections. It can queue up to 5 connection requests.
   */
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    printf("Listen failed: %s \n", strerror(errno));
    return 1;
  }

  printf("Waiting for a client to connect...\n");
  client_addr_len = sizeof(client_addr);
  /*
    The accept() function is called to accept an incoming connection. This will
    block (wait) until a client connects. */
  int client_socket;
  client_socket =
      accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

  if (client_socket == -1) {
    printf("Accept failed: %s \n", strerror(errno));
    return 1;
  }
  printf("Client connected\n");

  char buffer[BUFFER_SIZE] = {0};
  read(client_socket, buffer, BUFFER_SIZE);
  // printf("Received:\n%s", buffer);

  char *response = get_response(buffer);

  ssize_t bytes_sent = send(client_socket, response, strlen(response), 0);
  if (bytes_sent == -1) {
    printf("Send failed: %s \n", strerror(errno));
    return 1;
  } else {
    printf("Sent %zd bytes to client\n", bytes_sent);
  }

  printf("Closing client socket\n");
  close(client_socket);

  close(server_fd);

  return 0;
}
char *get_response(char *buffer) {
  static char response[MAX_RESPONSE_SIZE];

  if (strstr(buffer, "GET / ")) {
    return "HTTP/1.1 200 OK\r\n\r\n";
  } else if (strstr(buffer, "GET /echo/")) {
    // request is GET /echo/{str}, read str from buffer and return it on the
    // response body.
    char *str = strstr(buffer, "GET /echo/"); // get the pointer to the string.
    str += strlen("GET /echo/");              // move pointer to the end of the
                                              // string.
    char *end = strchr(str, ' ');             // find the end of the string.

    if (end == NULL) {
      return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }

    // Calculate the length of the echo string
    int str_len = end - str;

    // Prepare the response
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%.*s",
             str_len, str_len, str);

  } else if (strstr(buffer, "GET /user-agent")) {
    // TODO: read user-agent from headers and return it on the response body.

    char *user_agent = strstr(buffer, "User-Agent: ");
    user_agent += strlen("User-Agent: ");

    char *end = strchr(user_agent, '\r');
    if (end == NULL) {
      return "HTTP/1.1 400 Bad Request\r\n\r\n";
    }
    int user_agent_len = end - user_agent;
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%.*s",
             user_agent_len, user_agent_len, user_agent);

  } else {
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 404 Not Found\r\n"
             "\r\n");
  }

  return response;
}
