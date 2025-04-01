#ifndef WINPIPE_HPP
#define WINPIPE_HPP
#include <windows.h>
#include <iostream>
#include <cstring>

#define PIPE_NAME L"\\\\.\\pipe\\AurmpdPipe"
#define BUFFER_SIZE 1024

bool pipe_poll(struct thread_data *p );
#endif