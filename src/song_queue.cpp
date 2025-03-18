#include "song_queue.hpp"

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
void Song::setPos(int pos){
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
    j["queue_sid"] = m_queue_sid;
    j["coverArt"] = m_coverart;
    return j.dump();
}

// 定义 SongQueue 类来管理歌曲列表
// 添加歌曲到队列末尾
int SongQueue::addSong(Song& song) {
    // 发送addid命令并获取歌曲queue_id
    int queue_sid = mpd_run_add_id(mpd.conn, song.getURL().c_str());
    if (queue_sid < 0) {
        std::cerr<<"Failed to add song:"<<mpd_connection_get_error_message(mpd.conn)<<" song.url="<<song.getURL().c_str()<<std::endl;
        return -1;
    }else{
        song.setQueueSId(queue_sid);//add queue id
        m_songQueue.push_back(song);
        return queue_sid;
    }
}

// 通过查询queue根据id补全queue队列的pos信息
void SongQueue::makeupPos(){
    if (!mpd_send_list_queue_meta(mpd.conn)) {
        std::cerr << "Get queue failed: " << mpd_connection_get_error_message(mpd.conn) << std::endl;
        return;
    }    
    mpd_song *song;
    while ((song = mpd_recv_song(mpd.conn)) != nullptr) {
        unsigned int position = mpd_song_get_pos(song);
        unsigned int id = mpd_song_get_id(song);
        for (auto& asong : m_songQueue) {
            if(asong.getQueueSId()==id){
                asong.setPos(position);
                break;
            }
        }
        mpd_song_free(song);
    }
    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
        std::cerr << "recive queue failed: " << mpd_connection_get_error_message(mpd.conn) << std::endl;
    }

    mpd_response_finish(mpd.conn);
}

// 根据 id 获取歌曲
std::optional<Song> SongQueue::getSongById(const std::string& id) const {
    for (const auto& song : m_songQueue) {
        if (song.getId() == id) {
            return song;
        }
    }
    return std::nullopt;
}
// 删除指定 ID 的歌曲
bool SongQueue::removeSong(const std::string& id) {
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getId() == id) {
            m_songQueue.erase(it);
            return true;
        }
    }
    return false;
}

// 在指定位置插入歌曲
bool SongQueue::insertSong(int position, const Song& song) {
    if (position >= 0 && position <= static_cast<int>(m_songQueue.size())) {
        m_songQueue.insert(m_songQueue.begin() + position, song);
        return true;
    }
    return false;
}

// 清空歌曲队列
void SongQueue::clearQueue() {
    mpd_run_clear(mpd.conn);
    m_songQueue.clear();
}

// 获取队列中的歌曲数量
int SongQueue::getQueueSize() const {
    return static_cast<int>(m_songQueue.size());
}
//转换json格式文本
std::string  SongQueue::toJson() const {
    json j;
    for (const auto& song : m_songQueue) {
        j.push_back(json::parse(song.toJson()));
    }
    return j.dump();
}
////播放pos位置的歌曲
void SongQueue::playPos(int pos){
    for (const auto& song : m_songQueue){
        if(song.getPos() == pos){
            mpd_run_play_pos(mpd.conn,pos);
            break;
        }
    }
}
//根据queue_sid播放歌曲
void SongQueue::playSId(int queue_sid){
    for (const auto& song : m_songQueue){
        if(song.getQueueSId() == queue_sid){
            mpd_run_play_id(mpd.conn,queue_sid);
            break;
        }
    }
}

// 删除指定 queue_sid 的歌曲
bool SongQueue::removeSongBySid(const int queue_sid) {
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getQueueSId() == queue_sid) {
            m_songQueue.erase(it);
            mpd_run_delete_id(mpd.conn,queue_sid);
            return true;
        }
    }
    return false;
}
//队列queue长度
unsigned int SongQueue::length(){
    return m_songQueue.size();
}

// 迭代器相关方法实现
SongQueue::iterator SongQueue::begin() {
    return m_songQueue.begin();
}

SongQueue::iterator SongQueue::end() {
    return m_songQueue.end();
}

SongQueue::const_iterator SongQueue::begin() const {
    return m_songQueue.begin();
}

SongQueue::const_iterator SongQueue::end() const {
    return m_songQueue.end();
}

// main.cpp
/*
#include <iostream>
#include "SongQueue.h"

int main() {
    SongQueue songQueue;

    // 添加歌曲
    songQueue.addSong(Song("1", "Artist1", "Title1"));
    songQueue.addSong(Song("2", "Artist2", "Title2"));
    songQueue.addSong(Song("3", "Artist3", "Title3"));

    // 输出队列中的歌曲数量
    std::cout << "当前队列中的歌曲数量: " << songQueue.getQueueSize() << std::endl;

    // 插入歌曲
    songQueue.insertSong(1, Song("4", "Artist4", "Title4"));
    std::cout << "插入歌曲后，当前队列中的歌曲数量: " << songQueue.getQueueSize() << std::endl;

    // 删除歌曲
    if (songQueue.removeSong("2")) {
        std::cout << "成功删除 ID 为 2 的歌曲" << std::endl;
    } else {
        std::cout << "未找到 ID 为 2 的歌曲" << std::endl;
    }
    std::cout << "删除歌曲后，当前队列中的歌曲数量: " << songQueue.getQueueSize() << std::endl;

    // 清空队列
    songQueue.clearQueue();
    std::cout << "清空队列后，当前队列中的歌曲数量: " << songQueue.getQueueSize() << std::endl;

    return 0;
}
*/