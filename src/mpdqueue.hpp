#ifndef MPDQUEUE_HPP
#define MPDQUEUE_HPP
#include <iostream>
#include <optional>
#include <vector>
#include <string>
extern "C" {
    #include <mpd/client.h>
    #include "mpd_client.h"
}
#include "queue.hpp"
#include "json.hpp"

// 定义 Queue 类来管理mpd歌曲队列
class Mpdqueue : public Queue {
public:
    // 添加歌曲到队列末尾
    int addSong(Song& song);
    // 通过查询mpd根据id补全queue队列的pos信息
    void makeupPos();
    // 清空歌曲队列
    void clear();
    //播放pos位置的歌曲
    void playPos(int pos);
    //根据queue_sid播放歌曲
    void playSid(int queue_sid);
    // 删除指定 queue_sid 的歌曲
    bool removeSongBySid(const int queue_sid);
};

#endif