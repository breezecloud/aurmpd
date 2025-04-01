# aurmpd介绍
aurmpd名字的来源于[aurial](https://github.com/shrimpza/aurial)和[mpd](https://www.musicpd.org/)。aurial是一个基于浏览器的HTML/JavaScript客户端界面，用于从Subsonic、Airsonic、Navidrome或其他实现Subsonic API的软件和服务中流式传输音乐，无需使用Flash播放器或插件。我借鉴aurial的代码和[ympd](https://www.ympd.org/)的架构，开发了aurmpd音乐播放软件---一个可以基于subsonic协议在mpd上播放网络音乐或者本地音乐的软件。
## 特点
和aurial不同的地方：
1. 播放软件不使用HTML5的audio组件，而使用著名的开源音乐播放软件mpd，mpd本身是跨平台应用，理论上本软件也可以支持各种平台，目前支持windows和linux平台。
2. 嵌入了web服务器[mongoose](http://mongoose.ws)，在windows上已经集成mpd播放软件可以独立运行而不依赖任何其它软件，在linux上只需要配置mpd正常工作。
3. 得益于mpd，支持绝大多数音乐格式。
3. 播放音乐处于后台，可以关闭前台浏览器界面，不会引起音乐中断，同时播放队列也放在后台，所以关闭浏览器后会继续播放队列里的音乐。
4. 可以多个浏览器或不同终端上同时进行播放控制，多个播放界面之间状态同步。
## 安装&使用
1. 预编译包下载地址：https://github.com/breezecloud/aurmpd/releases
2. 本软件不需要安装，直接下载软件包解压到本机。windows平台已经集成了mpd软件，启动程序为winaurmpd.exe，启动后程序后台运行，状态栏有icon可以打开前端页面。在linux平台，确认电脑已经正确安装好mpd，启动程序为aurmpd。
3. windows平台的mpd配置文件是当前目录下的mpd.conf。在第一次启动winaurmpd.exe时会自动生成mpd.conf，其中输出设备是系统默认设备，本地音乐目录默认缺省为系统“音乐”文件夹。可以手动修改配置(参考模板说明)，需重新启动才能生效。Linux平台依赖之前已经安装好mpd配置，不需要设置。
4. 页面可以打开本机浏览器`http://127.0.0.1:8600`，或者从其它电脑打开浏览器`http://you.aurmpd.ip:8600`。
5. 在windows平台，程序在后台执行不会有任何信息显示；可以在命令行窗口手动执行winaurmpd.exe -d，这样程序不会退出命令并且会显示执行信息，方便问题查找。
## 编译
### 准备编译环境windows：
### 安装gcc
下载编译器(https://www.mingw-w64.org/)是将经典的开源C语言编译器GCC移植到了 Windows 平台下，并且包含了Win32API,还可以使用一些Windows不具备的Linux平台下的开发工具。
下载mingw：https://sourceforge.net/projects/mingw-w64/files/mingw-w64/mingw-w64-release/
下载w64devkit套件：https://github.com/skeeto/w64devkit/releases
w64devkit-x64-2.1.0.exe，套件包括：
1. MingW-W64 GCC：编译器，链接器，汇编器
2. GDB：调试器
3. GNU make：标准构建工具
4. BusyBox-W32：标准UNIX实用程序，包括SH
5. VIM：强大的文本编辑器
6. 通用CTAG：源导航
7. nasm：x86汇编器
8. CPPCHECK：静态代码分析
该工具链包括Pthreads，C ++ 11线程和OpenMP。所有包含的运行时组件都是静态的。
解压到c:\，会建立w64devkit目录，执行文件在bin目录下。
PATH添加C:\w64devkit\bin目录
#### 安装cmake
安装：
https://cmake.org/download/ 安装在C:\Program Files\CMake\ 执行文件bin目录下，cmake-gui是图形界面。
系统自动增加"C:\Program Files\CMake\bin"的PATH
说明：https://www.mingw-w64.org/build-systems/cmake/ 
文档 https://cmake.org/documentation/
#### 编译libmpdclient
windows环境需要自己编译libmpdclient库</br>
参考：https://github.com/MusicPlayerDaemon/libmpdclient</br>
安装ninja:https://github.com/ninja-build/ninja/releases 下载windos执行文件</br>
安装meason:python -m pip install meson 安装meson</br>
```
git clone https://github.com/MusicPlayerDaemon/libmpdclient
cd libmpdclient
meson setup output
ninja -C output
ninja -C output install #安装在c:/lib c:/include c:/share/doc/ c:/bin
```
### 准备编译环境linux（debian）：
```
sudo apt-get update
sudo apt-get install build-essential
sudo apt-get install libmpdclient-dev
```
### 下载源码
```
git clone https://github.com/breezecloud/aurmpd #download source code
```
源码分两部分，后端程序和前端页面，需要分开编译：
### 编译windows后端程序：
将c:/include/mpd文件夹复制到C:\w64devkit\include
```
cd aurmpd
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
make
```
编译生成执行文件后需要手动将libmpdclient-2.dll(在c:/bin)复制到执行目录
### 编译linux后端程序(debian)：

```
cd aurmpd
mkdir build
cd build
cmake ..
make
```
### 编译前端页面
```
install nodejs(https://nodejs.org/)
cd aurmal/aurial
npm install #for the first time
npm run dist #程序会编译并将编译结果复制到../build/htdocs，在启动winaurmpd.exe或者aurmpd后可以立即看到源码修改结果。
```

## 问题：
1. 由于在mpdclient不能在上一条命令执行还没反馈前再次执行下一条命令，所以不能操作过快，会引起和mpd后端应用的链接中断而需要重新链接,界面上会有延迟。
2. 暂时无法播放本地音乐。
3. playlist播放列表存储在本地浏览器缓存，无法在多个控制界面共享；同样认证信息也缓存在本地浏览器，无法在多个控制界面共享，需要在每个浏览器分别登录。
4. 在停止状态时无法完成“下一首”和“上一首”功能，在“暂停”状态时，点击“上一首”和“下一首”时会进入播放状态。
5. web界面手机浏览器端不兼容
## 改进计划
1. 播放本地音乐(已完成)
2. playlist改进(后台or服务器)
3. mpd本身的播放功能，包括：是否单曲循环、是否消费播放模式、是否重复播放、是否淡入淡出
4. 支持快进、快退
## changlog：
- 2025.3.29 version 0.2.0
增加了本地播放功能，修改了图标。
- 2025.3.1 version 0.1.0
第一个版本，完成了基本功能。
