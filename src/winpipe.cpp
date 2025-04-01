#include "winpipe.hpp"

bool pipe_poll(struct thread_data *p ) {
    // 创建命名管道
    bool right_flag = false;
    HANDLE hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        BUFFER_SIZE,
        BUFFER_SIZE,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        std::wcout << L"Failed to create named pipe. Error code: " << GetLastError() << std::endl;
        return false;
    }

    // 等待客户端连接
    if (ConnectNamedPipe(hPipe, NULL) != FALSE) {
        std::wcout << L"Client connected." << std::endl;

        char buffer[BUFFER_SIZE];
        DWORD bytesRead;
        DWORD totalBytesAvail;
        DWORD bytesLeftThisMessage;

        while (true) {
            // 使用 PeekNamedPipe 进行无阻塞检查
            if (PeekNamedPipe(hPipe, buffer, sizeof(buffer), &bytesRead, &totalBytesAvail, &bytesLeftThisMessage)) {
                if (totalBytesAvail > 0) {
                    // 有数据可供读取
                    if (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL)) {
                        buffer[bytesRead] = '\0';
                        std::wcout << L"Received message: " << buffer << std::endl;
                        if (std::strcmp(buffer, "CLOSE") == 0) {
                            std::wcout << L"Received CLOSE command, exiting loop." << std::endl;
                            right_flag = true;
                            break;
                        }
                    } else {
                        std::wcout << L"Failed to read from pipe. Error code: " << GetLastError() << std::endl;
                        break;
                    }
                }
            } else {
                std::wcout << L"PeekNamedPipe failed. Error code: " << GetLastError() << std::endl;
                break;
            }
            // 避免 CPU 空转
            Sleep(100);
        }
    } else {
        std::wcout << L"Failed to connect to client. Error code: " << GetLastError() << std::endl;
    }

    // 关闭句柄
    CloseHandle(hPipe);
    if(right_flag) 
        return true;
    else
        return false;
} 