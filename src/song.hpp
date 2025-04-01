#ifndef SONG_HPP
#define SONG_HPP
#include <string>
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
    //get coverart id
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
    void setPos(unsigned int pos); 
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
#endif