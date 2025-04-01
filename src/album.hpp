#ifndef ALBUM_HPP
#define ALBUM_HPP
#include <string>
#include <vector>
#include "queue.hpp"
#include "json.hpp"

// 定义 Album 类来表示专辑
class Album : public Queue {
public:
    // 构造函数
    Album(std::string& id,  std::string& artist,  std::string& name, int year);
    //增加歌曲到专辑
    void addSong(const Song &song);
    //返回专辑名称
    std::string getName() const;
    //返回专辑id
    std::string getId() const;    
    //to json
    std::string toJson() const;
    std::string albumToJson() const;
private:
    std::string m_id;
    std::string m_artist;
    std::string m_name;
    std::string m_coverart;    
    int m_year;
    int m_duration;
    int m_songCount;
};

#endif