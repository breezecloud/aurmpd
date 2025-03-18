# aurmpd介绍
aurmpd名字的来源于[aurial](https://github.com/shrimpza/aurial)和[mpd](https://www.musicpd.org/)。aurial是一个基于浏览器的HTML/JavaScript客户端界面，用于从Subsonic、Airsonic、Navidrome或其他实现Subsonic API的软件和服务中流式传输音乐，无需使用Flash播放器或插件。我借助aurial的代码和ympd(https://www.ympd.org/)的架构，开发了aurmpd音乐播放软件，和aurial不同的是不使用html5的audio控件，而是集成了mpd和一个web服务器[mongoose](http://mongoose.ws)。这样aurmpd借助mpd强大的音频播放能力，同时在windows上可以直接运行而不依赖任何其它软件，而在linux上需要mpd正常工作。
## 特点
1. 后端播放程序为著名的开源音乐播放软件mpd，前端为纯浏览器(websocket支持)。mpd本身是跨平台应用，目前应用支持windows和linux平台。
2. 可以支持多种音乐格式，大多数的主流音乐格式都支持，比如：mp3，flac，wav，dts等，取决于后端应用mpd对文件格式的支持。
3. 播放音乐处于后台，可以关闭前台浏览器界面，不会引起音乐中断，同时播放队列也放在后台，所以关闭浏览器后会继续播放队列里的音乐。
4. 可以使用多个浏览器或在不同终端上同时进行播放控制，多个播放界面之间状态同步。
## 安装
    预编译包下载地址：xxx
    本软件不需要安装，直接下载软件包解压到本机。window平台已经集成了mpd软件，可以直接运行winaurmpd.exe，启动后会自动打开浏览器。在linux平台，确认电脑已经正确安装好mpd，启动程序为时运行aurmpd。如果本机有窗口系统，打开本机浏览器http://127.0.0.1:8600，如果没有视窗系统，可以从别的电脑打开浏览器http://ip:8600.
## 配置
    本软件没有自己的配置文件，windows平台的mpd启动需要有一个配置文件是mpd.conf，在第一次启动会自动生成mpd.conf,mpd的目录默认在当前目录下，声卡默认为当前声卡。Linux平台依赖之前已经安装好的mpd配置，不需要调整。
## 编译
    windows环境：
    linux环境：
## 问题：
1. 由于在mpdclient不能在上一条命令执行还没反馈前再次执行下一条命令，所以不能操作过快，会引起和mpd后端应用的链接中断而需要重新链接,界面上会有延迟。
2. 暂时无法播放本地音乐。
3. playlist播放列表存储在本地浏览器缓存，无法在多个控制界面共享；同样认证信息也缓存在本地浏览器，无法在多个控制界面共享，需要在每个浏览器分别登录。
4. 在停止状态时无法完成“下一首”和“上一首”功能，在“暂停”状态时，点击“上一首”和“下一首”时会进入播放状态。
5. web界面手机浏览器端不兼容
## 改进计划
1. 播放本地音乐
2. playlist改进(后台or服务器)
3. mpd本身的播放功能，包括：是否单曲循环、是否消费播放模式、是否重复播放、是否淡入淡出
4. 支持快进、快退
changlog：
2025.3 version 0.1.0
第一个版本，完成了基本功能。