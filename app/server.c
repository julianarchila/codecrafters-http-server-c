#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// My includes
#include "request_parser.h"

#define BUFFER_SIZE 1024
#define MAX_RESPONSE_SIZE 102400
#define PATH_MAX 1024

// folderPath is char* of at most 100 characters

char folderPath[PATH_MAX] = {0};

struct client_data {
  int client_socket;
};

struct response {
  char *status;
  char *body;
};

char *get_response(char *buffer);
void *handle_client(void *arg);
int parse_arguments(int argc, char *argv[]);

int main(int argc, char *argv[]) {
  // Disable output buffering
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  if (parse_arguments(argc, argv) != 0) {
    return 1;
  }

  // Declare variables for the server file descriptor and client address
  // information.
  int server_fd;
  unsigned int client_addr_len;
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

  int client_socket;
  client_addr_len = sizeof(client_addr);

  // Set up a signal handler
  while (1) {
    /*
    The accept() function is called to accept an incoming connection. This will
    block (wait) until a client connects. */
    client_socket =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

    if (client_socket == -1) {
      printf("Accept failed: %s \n", strerror(errno));
      return 1;
    }

    printf("Client connected\n");

    struct client_data *data = malloc(sizeof(struct client_data));
    data->client_socket = client_socket;

    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, data) != 0) {
      printf("Failed to create thread: %s\n", strerror(errno));
      free(data);
      close(client_socket);
    } else {
      pthread_detach(thread_id);
    }
  }

  close(server_fd);

  return 0;
}

void *handle_client(void *arg) {
  struct client_data *data = (struct client_data *)arg;
  int client_socket = data->client_socket;

  char buffer[BUFFER_SIZE] = {0};
  read(client_socket, buffer, BUFFER_SIZE);

  char *response = get_response(buffer);

  send(client_socket, response, strlen(response), 0);

  close(client_socket);
  free(data);
  free(response);
  return NULL;
}

char *get_response(char *buffer) {
  // static char response[MAX_RESPONSE_SIZE];
  char *response = malloc(MAX_RESPONSE_SIZE);
  if (response == NULL) {
    // Handle memory allocation failure
    return NULL;
  }

  t_request req;
  if (parse_request(&req, buffer) != 0) {
    // TODO: handle parse_request error
    // return a http bad request response or internal server error
    return NULL;
  }

  // printf("Buffer: %s\n", buffer);
  printf("[line 174] Request: %s-\n", req.method);
  printf("[line 175] Path: %s-\n", req.path);
  printf("[line 176] Body: %s-\n", req.body);

  if (strcmp(req.method, "GET") == 0 && strcmp(req.path, "/") == 0) {
    // if (strstr(buffer, "GET / ")) {

    // return "HTTP/1.1 200 OK\r\n\r\n";
    printf("Handling GET /\n");
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "\r\n");

  } else if (strcmp(req.method, "GET") == 0 &&
             strstr(req.path, "/echo/") != NULL) {
    // request is GET /echo/{str}, read str from buffer and return it on the
    // response body.
    char *str = strstr(buffer, "GET /echo/"); // get the pointer to the string.
    str += strlen("GET /echo/");              // move pointer to the end of the
                                              // string.
    char *end = strchr(str, ' ');             // find the end of the string.

    if (end == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 400 Bad Request\r\n"
               "\r\n");
      return response;
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

    // } else if (strstr(buffer, "GET /user-agent")) {
  } else if (strcmp(req.method, "GET") == 0 &&
             strcmp(req.path, "/user-agent") == 0) {
    // TODO: read user-agent from headers and return it on the response body.

    char *user_agent = strstr(buffer, "User-Agent: ");
    user_agent += strlen("User-Agent: ");

    char *end = strchr(user_agent, '\r');
    if (end == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 400 Bad Request\r\n"
               "\r\n");
      return response;
    }
    int user_agent_len = end - user_agent;
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/plain\r\n"
             "Content-Length: %d\r\n"
             "\r\n"
             "%.*s",
             user_agent_len, user_agent_len, user_agent);

    // } else if (strstr(buffer, "GET /files/")) {
  } else if (strcmp(req.method, "GET") == 0 &&
             strstr(req.path, "/files/") != NULL) {
    // folderPath might be 'empty' (filed with zeroes), so we need to check
    // that it's not empty.
    if (folderPath[0] == '\0') {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 500 Internal Server Error\r\n"
               "\r\n");
      return response;
    }

    char *file_name =
        strstr(buffer, "GET /files/");  // get the pointer to the string.
    file_name += strlen("GET /files/"); // move pointer to the end of the
                                        // string.
    char *end = strchr(file_name, ' '); // find the end of the string.

    if (end == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 400 Bad Request\r\n"
               "\r\n");
      return response;
    }

    // Null-terminate the file name
    *end = '\0';

    // Construct the full file path
    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", folderPath, file_name);

    // Open the file
    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 404 Not Found\r\n"
               "\r\n");
      return response;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file content
    char *file_content = malloc(file_size);
    if (file_content == NULL) {
      fclose(file);
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 500 Internal Server Error\r\n"
               "\r\n");
      return response;
    }
    fread(file_content, 1, file_size, file);
    fclose(file);

    // Determine content type (you may want to expand this)
    const char *content_type = "application/octet-stream";
    content_type = "application/octet-stream";
    /* if (strstr(file_name, ".txt") != NULL) {
        content_type = "text/plain";
    } else if (strstr(file_name, ".html") != NULL) {
        content_type = "text/html";
    } */

    // Construct the response
    char *header = malloc(MAX_RESPONSE_SIZE);
    if (header == NULL) {
      free(file_content);
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 500 Internal Server Error\r\n"
               "\r\n");
      return response;
    }

    int header_length = snprintf(header, MAX_RESPONSE_SIZE,
                                 "HTTP/1.1 200 OK\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %ld\r\n"
                                 "\r\n",
                                 content_type, file_size);

    // Allocate memory for full response (header + file content)
    char *full_response = malloc(header_length + file_size);
    if (full_response == NULL) {
      free(file_content);
      free(header);
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 500 Internal Server Error\r\n"
               "\r\n");
      return response;
    }

    // Combine header and file content
    memcpy(full_response, header, header_length);
    memcpy(full_response + header_length, file_content, file_size);

    free(file_content);
    free(header);

    // Set the response to the full response
    // Note: This assumes that response is a pointer that can be reassigned
    response = full_response;

  } else if (strcmp(req.method, "POST") == 0 &&
             strstr(req.path, "/files/") != NULL) {
    // folderPath might be 'empty' (filed with zeroes), so we need to check
    // that it's not empty.
    if (folderPath[0] == '\0') {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 500 Internal Server Error\r\n"
               "\r\n");
      return response;
    }

    char *file_name =
        strstr(buffer, "POST /files/");  // get the pointer to the string.
    file_name += strlen("POST /files/"); // move pointer to the end of the
                                         // string.
    char *end = strchr(file_name, ' ');  // find the end of the string.

    if (end == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 400 Bad Request\r\n"
               "\r\n");
      return response;
    }

    // Null-terminate the file name
    *end = '\0';

    // Construct the full file path
    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", folderPath, file_name);

    // Open the file
    FILE *file = fopen(full_path, "w");
    if (file == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 404 Not Found\r\n"
               "\r\n");
      return response;
    }

    // parse body, req.body is everything after the path, which means the
    // headers and the body are separated by a blank line
    char *body = strstr(req.body, "\r\n\r\n");
    if (body == NULL) {
      snprintf(response, MAX_RESPONSE_SIZE,
               "HTTP/1.1 400 Bad Request\r\n"
               "\r\n");
      return response;
    }
    body += 4;

    // Write file content
    fwrite(body, 1, strlen(body), file);
    fclose(file);

    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 201 Created\r\n"
             "\r\n");
  } else {
    snprintf(response, MAX_RESPONSE_SIZE,
             "HTTP/1.1 404 Not Found\r\n"
             "\r\n");
  }

  free_request(&req);

  return response;
}

int parse_arguments(int argc, char *argv[]) {
  // TODO: --directory {name} might not be mandatory

  if (argc <= 1) {
    // printf("Usage: your_server.sh --directory /tmp/\n");
    return 0;
  }

  if (argc <= 2) {
    printf("Usage: your_server.sh --directory /tmp/\n");
    return 1;
  }

  if (strcmp(argv[1], "--directory") != 0) {
    printf("Usage: your_server.sh --directory /tmp/\n");
    return 1;
  }

  strcpy(folderPath, argv[2]);
  return 0;
}
