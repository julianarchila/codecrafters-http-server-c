#pragma once

typedef struct {
  char *method;
  char *path;
  char *body;
} t_request;

void free_request(t_request *req);

int parse_request(t_request *req, char *buffer);
