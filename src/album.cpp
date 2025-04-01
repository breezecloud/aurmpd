#include "album.hpp"

using json = nlohmann::json;

Album::Album(std::string& id,  std::string& artist,  std::string& name, int year)
    : m_id(id), m_artist(artist), m_name(name), m_year(year) {
    m_duration = 0;
    m_songCount = 0;
    m_coverart = "unknown";
}
//增加歌曲到专辑
void  Album::addSong(const Song &song){
    m_duration += song.getDuration();
    m_songCount++;
    Queue::addSong(song);
}
//返回专辑名称
std::string Album::getName() const{
    return m_name;
}
//返回专辑id
std::string Album::getId() const{
    return m_id;
}
//to json
std::string Album::toJson() const {
    json j;
    j["id"] = m_id;
    j["artist"] = m_artist;
    j["name"] = m_name;
    j["coverart"] = m_coverart;
    j["year"] = m_year;
    j["duration"] = m_duration;
    j["songCount"] = m_songCount;   
    // 调用基类的 toJson 方法并解析结果.不能直接用songJsonStr，否则引号"转成\"
    std::string songJsonStr = static_cast<const Queue*>(this)->toJson();
    try {
        json songJson = json::parse(songJsonStr);
            j["song"] = songJson;
    } catch (const json::parse_error& e) {
        std::cerr << "Failed to parse song JSON: " << e.what() << std::endl;
        j["song"] = json::object();
    }
    return j.dump();
}
//to json
std::string Album::albumToJson() const {
    json j;
    j["id"] = m_id;
    j["artist"] = m_artist;
    j["name"] = m_name;
    j["coverart"] = m_coverart;
    j["year"] = m_year;
    j["duration"] = m_duration;
    j["songCount"] = m_songCount;
    return j.dump();
}