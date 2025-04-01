#include "song.hpp"

using json = nlohmann::json;

Song::Song(std::string& id,  std::string& artist,  std::string& title,  std::string& album,  std::string& url)
    : m_id(id), m_artist(artist), m_title(title), m_album(album), m_url(url) {}
// 获取歌曲 ID 实现
std::string Song::getId() const {
    return m_id;
}
// 获取歌手实现
std::string Song::getArtist() const {
    return m_artist;
}
// 获取歌曲名字实现
std::string Song::getTitle() const {
    return m_title;
}
// 获取专辑实现
std::string Song::getAlbum() const {
    return m_album;
}
// 获取歌曲URL实现
std::string Song::getURL() const {
    return m_url;
}
//get coverart  song id
std::string Song::getCoverart() const{
    return m_coverart;
}
//set coverart
void Song::setCoverart(std::string coverart){
    m_coverart = coverart;
}
//get mpd queue id
int Song::getQueueSId() const {
    return m_queue_sid;
}
//set mpd queue id
void Song::setQueueSId(int queue_sid){
    m_queue_sid = queue_sid;
}
//get track
int Song::getTrack() const {
    return m_track;
}
//set track
void Song::setTrack(int track){
    m_track = track;
}
//get year
int Song::getYear() const {
    return m_year;
}
//set year
void Song::setYear(int year){
    m_year = year;
}
//get duration
int Song::getDuration() const {
    return m_duration;
}
//set duration
void Song::setDuration(int duration){
    m_duration = duration;
}
//get pos
int Song::getPos()const {
   return m_pos;
}
void Song::setPos(unsigned int pos){
    m_pos= pos;
}
//tojson
std::string Song::toJson() const {
    json j;
    j["id"] = m_id;
    j["artist"] = m_artist;
    j["title"] = m_title;
    j["album"] = m_album;
    j["url"] = m_url;
    j["track"] = m_track;
    j["year"] = m_year;
    j["duration"] = m_duration;
    j["pos"] = m_pos;
    if(m_queue_sid>=0)//-1不转queue_sid，否则会和queue默认停止时的id值混淆
        j["queue_sid"] = m_queue_sid;
    j["coverArt"] = m_coverart;
    return j.dump();
}
