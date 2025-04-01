#ifndef QUEUE_HPP
#define QUEUE_HPP
#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include "song.hpp"
#include "json.hpp"

// 定义 Queue 类来管理歌曲队列
class Queue {
public:
    // 添加歌曲到队列末尾
    void addSong(const Song& song);
    // 根据 id 获取歌曲
    std::optional<Song> getSongById(const std::string& id) const;
    // 根据 queue_sid 获取歌曲
    std::optional<Song> getSongBySId(unsigned int sid) const;
    // 根据 pos 获取歌曲
    std::optional<Song> getSongByPos(unsigned int pos) const;    
    // 删除指定 ID 的歌曲
    bool removeSongByid(const std::string& id);
    // 删除指定 queue_sid 的歌曲
    bool removeSongBySid(const int sid);    
    // 删除指定歌曲
    bool erase(const Song& song);
    // 找到相同queue_sid的歌曲并替换
    bool updateSonge(const Song& song);
    // 清空歌曲队列
    void clear();
    // 获取队列中的歌曲数量
    int getQueueSize() const;
    //队列queue长度
    unsigned int length();
    //转换json格式文本
    std::string toJson() const;
    // 迭代器相关方法
    using iterator = std::vector<Song>::iterator;
    using const_iterator = std::vector<Song>::const_iterator;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    
private:
    std::vector<Song> m_songQueue;
};

#endif