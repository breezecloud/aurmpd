#ifndef SONGQUEUE_HPP
#define SONGQUEUE_HPP
#include <iostream>
#include <optional>
#include <vector>
#include <string>
extern "C" {
    #include <mpd/client.h>
    #include "mpd_client.h"
}

#include "json.hpp"

// 定义 Song 类来表示一首歌曲
class Song {
public:
    // 构造函数
    Song(std::string& id,  std::string& artist,  std::string& title, std::string& album, std::string& url);
    // 获取歌曲 ID
    std::string getId() const;
    // 获取歌手
    std::string getArtist() const;
    // 获取歌曲名称
    std::string getTitle() const;
    // 获取专辑名称
    std::string getAlbum() const;
    // 获取歌曲URL
    std::string getURL() const;
    //get coverart  song id
    std::string getCoverart() const;
    //set coverart
    void setCoverart(std::string);    
    //get mpd queue song id
    int getQueueSId() const;
    //set mpd queue id
    void setQueueSId(int queue_sid);
    //get track
    int getTrack() const;
    //set track
    void setTrack(int track);
    //get year
    int getYear() const;
    //set year
    void setYear(int year);
    //get duration
    int getDuration() const;
    //set duration
    void setDuration(int duration); 
    //get pos
    int getPos() const;   
    //set pos
    void setPos(int pos); 
    //toJson
    std::string toJson() const;
private:
    std::string m_id;
    std::string m_artist;
    std::string m_title;
    std::string m_album;
    std::string m_url;
    std::string m_coverart;    
    int m_track;
    int m_year;
    int m_duration;
    int m_pos = -1;
    int m_queue_sid = -1;
};

// 定义 SongQueue 类来管理歌曲列表
class SongQueue {
public:
    // 添加歌曲到队列末尾
    int addSong(Song& song);
    // 通过查询mpd根据id补全queue队列的pos信息
    void makeupPos();
    // 根据 id 获取歌曲
    std::optional<Song> getSongById(const std::string& id) const;
    // 删除指定 ID 的歌曲
    bool removeSong(const std::string& id);
    // 在指定位置插入歌曲
    bool insertSong(int position, const Song& song);
    // 清空歌曲队列
    void clearQueue();
    // 获取队列中的歌曲数量
    int getQueueSize() const;
    //转换json格式文本
    std::string toJson() const;
    //播放pos位置的歌曲
    void playPos(int pos);
    //根据queue_sid播放歌曲
    void playSId(int queue_sid);
    // 删除指定 queue_sid 的歌曲
    bool removeSongBySid(const int queue_sid);
    //队列queue长度
    unsigned int length();

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