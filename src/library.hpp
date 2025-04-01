#ifndef LIBRARY_HPP
#define LIBRARY_HPP
#include <iostream>
#include <optional>
#include <vector>
#include <string>
extern "C" {
    #include <mpd/client.h>
    #include "mpd_client.h"
}

#include "song.hpp"
#include "queue.hpp"
#include "album.hpp"
#include "json.hpp"


// 定义 Library 类，管理本地mpd库，继承自 Queue 类
class Library {
public:
    //清除历史记录
    void clear();
    //读入mpd本地db
    bool getMpdDB();
    //找到某专辑
    bool findAlbum(std::string id);
    //新增加专辑
    bool addAlbum(Album &album);
    //某专辑新增歌曲
    bool addSong(std::string id,const Song &song);
    //所有专辑转json（不包括song）
    std::string  allAlbumToJson() const;
    //某专辑的song转json
    std::string  AlbumToJson(std::string id) const;    
    // 迭代器相关方法
    using iterator = std::vector<Album>::iterator;
    using const_iterator = std::vector<Album>::const_iterator;
    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;        
private:
    std::vector<Album> m_albumQueue;
};
#endif