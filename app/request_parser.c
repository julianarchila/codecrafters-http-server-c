#include "request_parser.h"
#include <stdlib.h>
#include <string.h>

int parse_request(t_request *req, char *buffer) {
  // Parse the method
  char *method_end = strstr(buffer, " ");
  if (method_end == NULL) {
    return -1;
  }
  char *method = malloc(method_end - buffer + 1);
  strncpy(method, buffer, method_end - buffer);
  method[method_end - buffer] = '\0';

  /* printf("method_end: %s\n", method_end);
  printf("method len: %ld\n", method_end - buffer); */

  req->method = method;

  // Parse the path
  method_end += sizeof(char);
  char *path_end = strstr(method_end, " ");
  if (path_end == NULL) {
    return -1;
  }
  char *path = malloc(path_end - method_end + 1);
  strncpy(path, method_end, path_end - method_end);
  path[path_end - method_end] = '\0';
  req->path = path;

  // Parse the body aka everything after the path (for now)
  req->body = path_end + sizeof(char);

  return 0;
}

void free_request(t_request *req) {
  if (req == NULL) {
    return;
  }

  free(req->method);
  free(req->path);
  // free(req->body);
  // free(req);
}
