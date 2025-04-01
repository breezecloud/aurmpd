#include "queue.hpp"

using json = nlohmann::json;

// 定义 Queue 类来管理歌曲列表
// 添加歌曲到队列末尾
void Queue::addSong(const Song& song) {
        m_songQueue.push_back(song);
}

// 根据 id 获取歌曲
std::optional<Song> Queue::getSongById(const std::string& id) const {
    for (const auto& song : m_songQueue) {
        if (song.getId() == id) {
            return song;
        }
    }
    return std::nullopt;
}

// 根据 queue_sid 获取歌曲
std::optional<Song> Queue::getSongBySId(unsigned int sid) const{
    for (const auto& song : m_songQueue) {
        if (song.getQueueSId() == sid) {
            return song;
        }
    }
    return std::nullopt;    
}

// 根据 pos 获取歌曲
std::optional<Song> Queue::getSongByPos(unsigned int pos) const{
    for (const auto& song : m_songQueue) {
        if (song.getPos() == pos) {
            return song;
        }
    }
    return std::nullopt;       
}

// 删除指定 ID 的歌曲
bool Queue::removeSongByid(const std::string& id) {
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getId() == id) {
            m_songQueue.erase(it);
            return true;
        }
    }
    return false;
}

// 删除指定 queue_sid 的歌曲
bool  Queue::removeSongBySid(const int sid){
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getQueueSId() == sid){
            m_songQueue.erase(it);
            return true;
        }
    }
    return false;    
}

// 删除指定歌曲
bool  Queue::erase(const Song& song){
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getId() == song.getId()) {
            m_songQueue.erase(it);
            return true;
        }
    }
    return false;
}
// 找到相同queue_sid的song并替换
bool Queue::updateSonge(const Song& song){
    for (auto it = m_songQueue.begin(); it != m_songQueue.end(); ++it) {
        if (it->getQueueSId() == song.getQueueSId()) {
            m_songQueue.erase(it);
            addSong(song);
            return true;
        }
    }
    return false; 
}

// 清空歌曲队列
void Queue::clear() {
    m_songQueue.clear();
}

// 获取队列中的歌曲数量
int Queue::getQueueSize() const {
    return static_cast<int>(m_songQueue.size());
}
//转换json格式文本
std::string  Queue::toJson() const {
    json j;
    if(getQueueSize()){
        for (const auto& song : m_songQueue) {
            j.push_back(json::parse(song.toJson()));
        }
        return j.dump();
    }else
        return std::string("[]");//return empty not return null
}
//队列queue长度
unsigned int Queue::length(){
    return m_songQueue.size();
}

// 迭代器相关方法实现
Queue::iterator Queue::begin() {
    return m_songQueue.begin();
}

Queue::iterator Queue::end() {
    return m_songQueue.end();
}

Queue::const_iterator Queue::begin() const {
    return m_songQueue.begin();
}

Queue::const_iterator Queue::end() const {
    return m_songQueue.end();
}
