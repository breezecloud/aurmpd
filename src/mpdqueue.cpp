#include "mpdqueue.hpp"

using json = nlohmann::json;

// 定义 Mpdqueue 类来管理mpd列表
// 添加歌曲到队列末尾
int Mpdqueue::addSong(Song& song) {
    // 发送addid命令并获取歌曲queue_id
    int queue_sid = mpd_run_add_id(mpd.conn, song.getURL().c_str());
    if (queue_sid < 0) {
        std::cerr<<"Failed to add song:"<<mpd_connection_get_error_message(mpd.conn)<<" song.url="<<song.getURL().c_str()<<std::endl;
        return -1;
    }else{
        song.setQueueSId(queue_sid);//add queue id
        Queue::addSong(song);
        return queue_sid;
    }
}

// 通过查询queue根据id补全queue队列的pos信息
void Mpdqueue::makeupPos(){
    if (!mpd_send_list_queue_meta(mpd.conn)) {
        std::cerr << "Get queue failed: " << mpd_connection_get_error_message(mpd.conn) << std::endl;
        return;
    }    
    mpd_song *song;
    while ((song = mpd_recv_song(mpd.conn)) != nullptr) {
        unsigned int position = mpd_song_get_pos(song);
        unsigned int sid = mpd_song_get_id(song);
        std::optional<Song> m_song = getSongBySId(sid);
        if(m_song){
            (*m_song).setPos(position);
            updateSonge(*m_song);
        }
        mpd_song_free(song);
    }
    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
        std::cerr << "recive queue failed: " << mpd_connection_get_error_message(mpd.conn) << std::endl;
    }
    //mpd_response_finish(mpd.conn);
}

// 清空歌曲队列
void Mpdqueue::clear() {
    mpd_run_clear(mpd.conn);
    Queue::clear();
}

////播放pos位置的歌曲
void Mpdqueue::playPos(int pos){
    if(getSongByPos(pos))
        mpd_run_play_pos(mpd.conn,pos);
}

//根据queue_sid播放歌曲
void Mpdqueue::playSid(int queue_sid){
    if(getSongBySId(queue_sid))
            mpd_run_play_id(mpd.conn,queue_sid);
}

// 删除指定 queue_sid 的歌曲
bool Mpdqueue::removeSongBySid(const int queue_sid) {
    if(Queue::removeSongBySid(queue_sid)){
        if (mpd_run_delete_id(mpd.conn, queue_sid)) {
            return true;
        } else {
            std::cerr << "Failed to delete song from MPD: " << mpd_connection_get_error_message(mpd.conn) << std::endl;
            // 这里可以考虑恢复本地队列，避免数据不一致
            return false;
        }
    }
    return false;
}
