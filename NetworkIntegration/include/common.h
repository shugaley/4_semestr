#pragma once

#include <stdio.h>

#define UDP_PORT 8001
#define TCP_PORT 8002

enum error {
    SUCCESS = 0,
    E_INVAL,
    E_SOCK,
    E_CONN,
    E_MEM,
    E_THREAD,
};

struct start_pack {
    int server;
    size_t n_threads;
    size_t n_machines;
};

void p_error (enum error err);
void run_server(size_t n_threads, size_t n_machines);
void run_worker ();
int set_keepalive (int socket, int keepcnt, int keepidle, int keepintvl);