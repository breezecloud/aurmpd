#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <string.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <functiondiscoverykeys_devpkey.h>

// 自定义布尔类型
typedef int bool;
#define true 1
#define false 0

// 定义托盘图标 ID
#define IDI_AurmpdICON 1
// 定义菜单 ID
#define IDM_EXIT 100
#define IDM_OPEN 101
#define IDM_ABOUT 102

#define TARGET_URL L"http://127.0.0.1:8600"
#define PIPE_NAME L"\\\\.\\pipe\\AurmpdPipe"
#define BUFFER_SIZE 1024

// 全局变量，用于存储窗口句柄
HWND hWnd;
// 全局变量，用于存储子进程的进程句柄
HANDLE hChildProcess_mpd = NULL;
HANDLE hChildProcess_aurmpd = NULL;

// 前置声明 GracefullyCloseProcess 函数
BOOL GracefullyCloseProcess(HANDLE hProcess);


// 获取默认音频播放设备名称
BOOL GetDefaultAudioDeviceName(wchar_t* deviceName, size_t bufferSize) {
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnumerator = NULL;
    IMMDevice* defaultDevice = NULL;
    IPropertyStore* propertyStore = NULL;
    PROPVARIANT varName;
    // 手动定义GUID
    static const GUID CLSID_MMDeviceEnumerator = {0xbcde0395, 0xe52f, 0x467c, {0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e}};
    static const GUID IID_IMMDeviceEnumerator = {0xa95664d2, 0x9614, 0x4f35, {0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6}};
    // 初始化 COM 库
    hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        return FALSE;
    }
    // 创建设备枚举器实例
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (LPVOID*)&deviceEnumerator);
    if (FAILED(hr)) {
        CoUninitialize();
        return FALSE;
    }
    // 获取默认音频渲染设备
    hr = deviceEnumerator->lpVtbl->GetDefaultAudioEndpoint(deviceEnumerator, eRender, eConsole, &defaultDevice);
    if (FAILED(hr)) {
        deviceEnumerator->lpVtbl->Release(deviceEnumerator);
        CoUninitialize();
        return FALSE;
    }
    // 获取设备属性存储
    hr = defaultDevice->lpVtbl->OpenPropertyStore(defaultDevice, STGM_READ, &propertyStore);
    if (FAILED(hr)) {
        defaultDevice->lpVtbl->Release(defaultDevice);
        deviceEnumerator->lpVtbl->Release(deviceEnumerator);
        CoUninitialize();
        return FALSE;
    }
    // 初始化属性值变量
    PropVariantInit(&varName);
    // 获取设备友好名称
    hr = propertyStore->lpVtbl->GetValue(propertyStore, &PKEY_Device_FriendlyName, &varName);
    if (SUCCEEDED(hr)) {
        if (varName.vt == VT_LPWSTR) {
            wcsncpy_s(deviceName, bufferSize, varName.pwszVal, _TRUNCATE);
        }
        PropVariantClear(&varName);
    }
    // 释放资源
    propertyStore->lpVtbl->Release(propertyStore);
    defaultDevice->lpVtbl->Release(defaultDevice);
    deviceEnumerator->lpVtbl->Release(deviceEnumerator);
    CoUninitialize();
    return SUCCEEDED(hr);
}

// 检查并复制文件
BOOL CheckAndCopyFiles() {
    FILE *src, *dst;
    char buffer[1024];
    size_t bytesRead;
    wchar_t deviceName[256];

    // 检查 mpd.conf 文件是否存在
    if (GetFileAttributes("mpd.conf") == INVALID_FILE_ATTRIBUTES) {
        // 检查 mpd.conf.tmp 文件是否存在
        if (GetFileAttributes("mpd.conf.tmp") != INVALID_FILE_ATTRIBUTES) {
            // 打开源文件和目标文件
            src = fopen("mpd.conf.tmp", "rb");
            dst = fopen("mpd.conf", "wb");
            if (src == NULL || dst == NULL) {
                MessageBox(NULL, "Failed to open mpd.conf files", "Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }
            if (src && dst) {
                // 复制文件内容
                while ((bytesRead = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytesRead, dst);
                }

                // 获取默认音频播放设备名称
                if (GetDefaultAudioDeviceName(deviceName, sizeof(deviceName) / sizeof(wchar_t))) {
                    // 显示 deviceName 的值
                    //MessageBoxW(NULL, deviceName, L"Default Audio Device Name", MB_OK | MB_ICONINFORMATION);
                    const char* audio_output_start = "\naudio_output {\n    type \"winmm\"\n    name \"";
                    fwrite(audio_output_start, strlen(audio_output_start), 1, dst);
                    // 转换为 UTF - 8 并写入文件
                    int len = WideCharToMultiByte(CP_UTF8, 0, deviceName, -1, NULL, 0, NULL, NULL);
                    char* utf8DeviceName = (char*)malloc(len+1);
                    if (utf8DeviceName) {
                        WideCharToMultiByte(CP_UTF8, 0, deviceName, -1, utf8DeviceName, len, NULL, NULL);
                        fwrite(utf8DeviceName, 1, len -1 , dst);
                        free(utf8DeviceName);
                    }
                    const char* audio_output_end = "\"\n}\n";
                    fwrite(audio_output_end, strlen(audio_output_end), 1, dst);                    
                }

                // 关闭文件
                fclose(src);
                fclose(dst);
                return TRUE;
            } else {
                if (src) fclose(src);
                if (dst) fclose(dst);
                return FALSE;
            }
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

// 处理窗口消息的回调函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            if (!CheckAndCopyFiles()) {
                MessageBox(hwnd, "Failed to copy or create mpd.conf", "Error", MB_OK | MB_ICONERROR);
            }            
            // 创建系统托盘图标
            NOTIFYICONDATA nid;
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwnd;
            nid.uID = IDI_AurmpdICON;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_USER + 1;
            // 加载自定义图标
            HICON hIcon = (HICON)LoadImage(
                GetModuleHandle(NULL),
                //MAKEINTRESOURCE(IDI_AurmpdICON),
                "IDI_AurmpdICON",
                IMAGE_ICON,
                0, 0, // 指定需要的尺寸
                LR_DEFAULTCOLOR
            );
            if (hIcon == NULL) {
                DWORD err = GetLastError();
                TCHAR msg[256];
                FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, msg, 256, NULL);
                MessageBox(NULL, msg, "Failed to load icon", MB_OK | MB_ICONERROR);
                hIcon = LoadIcon(NULL, IDI_APPLICATION);
            }
            nid.hIcon = hIcon; 
            // 设置托盘图标提示信息
            lstrcpy(nid.szTip, "Aurmpd Application");
            Shell_NotifyIcon(NIM_ADD, &nid);

            // 启动后自动最小化到系统托盘
            ShowWindow(hwnd, SW_MINIMIZE);

            // 启动子进程
            STARTUPINFO si = { sizeof(STARTUPINFO) };
            si.cb = sizeof(STARTUPINFO);
            si.wShowWindow = SW_HIDE; // 隐藏子进程窗口
            si.dwFlags |= STARTF_USESHOWWINDOW;
            PROCESS_INFORMATION pi;
            // 创建mpd子进程   
            char command1[] = "mpd.exe mpd.conf";
            if (!CreateProcess(NULL, command1, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                MessageBox(hwnd, "Failed to start mpd process", "Error", MB_OK | MB_ICONERROR);
            } else {
                // 保存子进程的句柄
                hChildProcess_mpd = pi.hProcess;
                // 关闭线程句柄，因为我们不需要它
                CloseHandle(pi.hThread);           
            }
            // 创建aurmpd子进程   
            char command2[] = "aurmpd.exe";
            if (!CreateProcess(NULL, command2, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                MessageBox(hwnd, "Failed to start mpd process", "Error", MB_OK | MB_ICONERROR);
            } else {
                // 保存子进程的句柄
                hChildProcess_aurmpd = pi.hProcess;
                // 关闭线程句柄，因为我们不需要它
                CloseHandle(pi.hThread);                 
            }
            
            // 启动后打开浏览器并访问指定网址
            ShowWindow(hwnd, SW_HIDE); 
            ShellExecuteW(hwnd, L"open", TARGET_URL, NULL, NULL, SW_SHOWNOACTIVATE);
            break;
        }
        case WM_DESTROY: {          
            // 移除系统托盘图标
            NOTIFYICONDATA nid = { sizeof(NOTIFYICONDATA) };
            nid.hWnd = hwnd;
            nid.uID = IDI_AurmpdICON;
            if (!Shell_NotifyIcon(NIM_DELETE, &nid)) {
                MessageBox(hwnd, "Failed to remove tray icon", "Error", MB_OK | MB_ICONERROR);
            }

            // 通过命名管道发送CLOSE命令关闭aurmpd.exe子进程
            if (hChildProcess_aurmpd && (hChildProcess_aurmpd != INVALID_HANDLE_VALUE)) {
                HANDLE hPipe = CreateFileW(
                    PIPE_NAME,
                    GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                );
                //打开成功
                if (hPipe != INVALID_HANDLE_VALUE) {
                    const char* message = "CLOSE";
                    DWORD bytesWritten;
                    if (WriteFile(hPipe, message, (DWORD)strlen(message), &bytesWritten, NULL)) {
                        wprintf(L"Message sent successfully.\n");
                    } else {
                        wprintf(L"Failed to send message. Error code: %lu\n", GetLastError());
                    }
                    // 关闭句柄
                    CloseHandle(hPipe);                
                }else{//打开pipe失败强制关闭
                    GracefullyCloseProcess(hChildProcess_aurmpd);
                }                 
            }
            Sleep(2000);//waiting 2s
            if (hChildProcess_mpd && hChildProcess_mpd != INVALID_HANDLE_VALUE) {
                GracefullyCloseProcess(hChildProcess_mpd);
            }
            // 关闭进程句柄
            if (hChildProcess_mpd != NULL) {
                CloseHandle(hChildProcess_mpd);
            }
            if (hChildProcess_aurmpd != NULL) {
                CloseHandle(hChildProcess_aurmpd);
            }
    
            PostQuitMessage(0);
            break;
        }
        case WM_USER + 1: {
            // 处理托盘图标消息
            switch (LOWORD(lParam)) {
                case WM_RBUTTONUP: {
                    // 创建弹出菜单
                    HMENU hMenu = CreatePopupMenu();
                    AppendMenu(hMenu, MF_STRING, IDM_OPEN, "Open");
                    AppendMenu(hMenu, MF_STRING, IDM_ABOUT, "About");
                    AppendMenu(hMenu, MF_STRING, IDM_EXIT, "Exit");

                    // 获取鼠标位置
                    POINT pt;
                    GetCursorPos(&pt);

                    // 显示弹出菜单，避免激活主窗口
                    UINT cmd = TrackPopupMenuEx(hMenu, TPM_RIGHTBUTTON | TPM_NOANIMATION | TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, hwnd, NULL);
                    if (cmd > 0) {
                        SendMessage(hwnd, WM_COMMAND, cmd, 0);
                    }
                    DestroyMenu(hMenu);
                    break;
                }
            }
            break;
        }
        case WM_COMMAND: {
            // 处理菜单命令
            switch (LOWORD(wParam)) {
                case IDM_EXIT: {                 
                    // 退出应用程序
                    DestroyWindow(hwnd);
                    break;
                }
                case IDM_OPEN: {
                    // 打开浏览器并访问指定网址
                    ShowWindow(hwnd, SW_HIDE); 
                    ShellExecuteW(hwnd, L"open", TARGET_URL, NULL, NULL, SW_SHOWNOACTIVATE);
                    break;
                }
                case IDM_ABOUT: {
                    const wchar_t* aboutText = L"aurmpd music player version 0.2.0\n"
                                               L"By luping(luping@189.cn)\n"
                                               L"https://github.com/breezecloud/aurmpd/";
                    // 确保主窗口隐藏
                    ShowWindow(hwnd, SW_HIDE);                                               
                    MessageBoxW(hwnd, aboutText, L"About",MB_OK | MB_ICONINFORMATION | MB_TOPMOST | MB_SETFOREGROUND);
                    break;
                }                
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 优雅关闭子进程
BOOL GracefullyCloseProcess(HANDLE hProcess) {
    if (hProcess == NULL) {
        return TRUE;
    }

    // 获取进程 ID
    DWORD dwProcessId = GetProcessId(hProcess);
    if (dwProcessId == 0) {
        return FALSE;
    }

    // 尝试向进程发送关闭信号
    /*
    if (GenerateConsoleCtrlEvent(CTRL_C_EVENT, dwProcessId)) {        
        // 等待进程退出
        if (WaitForSingleObject(hProcess, 5000) == WAIT_OBJECT_0) { 
            return TRUE;
        }
    }
    */
    // 强制终止进程
    return TerminateProcess(hProcess, 0);
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool debugMode = false;

    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"-d") == 0) {
            debugMode = true;
            break;
        }
    }

    if (!debugMode) {
        // 在后台执行
        /*
        if (GetConsoleWindow()) {
            FreeConsole();
        }*/
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        wchar_t szCommandLine[MAX_PATH];
        GetModuleFileNameW(NULL, szCommandLine, MAX_PATH);
        wcscat_s(szCommandLine, MAX_PATH, L" -d");
        if (CreateProcessW(NULL, szCommandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi)) {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 0;
        }
    }

    LocalFree(argv); 
       
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "AurmpdAppClass";
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AurmpdICON));
    
    if (wc.hIcon == NULL) {
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); // 使用默认图标作为备用
    }
    RegisterClass(&wc);

    // 创建窗口，使用 WS_EX_TOOLWINDOW WS_POPUP 样式并设置大小为 0x0，使得窗口不可见
    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, wc.lpszClassName, "Aurmpd Application", WS_POPUP | WS_EX_TOOLWINDOW,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL);        

    if (hWnd == NULL) {
        return 0;
    }
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);  
    }
    return msg.wParam;
}