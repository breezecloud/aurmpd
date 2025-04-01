1. 测试windows环境下pipe,client会发送一个命令CLOSE，server收到命令会显示收到的命令并退出
server.cpp
client.c
编译：
g++ server.cpp -o server
gcc client.c -o client
执行：先运行server.exe 在运行client.exe
2. 读取音乐库信息
mpd_library_reader.cpp
编译：g++ -o mpd_library_reader mpd_library_reader.cpp -lmpdclient -I C:\w64devkit\include
执行：安装mpdclient，并且libmpdclient-2.dll复制的当前目录