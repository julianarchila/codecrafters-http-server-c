#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
      .sin_addr = {htonl(INADDR_ANY)},
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

  const char *message = "HTTP/1.1 200 OK\r\n\r\n";

  ssize_t bytes_sent = send(client_socket, message, strlen(message), 0);
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
