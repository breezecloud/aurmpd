#include <iostream>
#include <string>
#include "http_server.hpp"
#include "json.hpp"
#include "song_queue.hpp"

using json = nlohmann::json;

SongQueue SQ;

//根据post内容加入track到mpd的queue，并且返回第一个加入的queue_sid;
int addTrackTompd(struct mg_http_message *hm){
    json jsonArray;
    int queue_sid = -1;
    int queue_sid_t = -1;

    if(SQ.length() == 0) //如果SQ中没有歌曲，清空mpd的queue，以保持mpd和SQ的同步。
        mpd_run_clear(mpd.conn);
    try {
        // 解析 JSON 字符串
        std::string json_str = std::string(hm->body.buf,(int) hm->body.len);
        jsonArray = json::parse(json_str);
        //std::cout<<jsonArray<<"\n";
    } catch (const json::parse_error& e) {
        // 处理解析错误
        std::cerr << "JSON parsing error: " << e.what() << std::endl;
    }                   
        // 检查解析结果是否为数组
    if (jsonArray.is_array()) {
        // 遍历 JSON 数组
        for (const auto& item : jsonArray) {
            // 检查每个元素是否为对象
            if (item.is_object()) {
                std::string id = "";
                std::string title = "";
                std::string album = "";
                std::string artist = "";
                std::string url = "";
                std::string coverArt = "";
                int duration = 0;
                int track = 0;
                int year = 0;
                // 检查键是否存在并提取值
                if (item.contains("id") && item["id"].is_string())
                    id = item["id"].get<std::string>();
                if (item.contains("title") && item["title"].is_string())
                    title = item["title"].get<std::string>();
                if (item.contains("album") && item["album"].is_string())
                    album = item["album"].get<std::string>();
                if (item.contains("artist") && item["artist"].is_string())
                    artist = item["artist"].get<std::string>();
                if (item.contains("url") && item["url"].is_string())
                    url = item["url"].get<std::string>();
                if (item.contains("duration") && item["duration"].is_number_integer())
                    duration = item["duration"].get<int>();
                if (item.contains("track") && item["track"].is_number_integer())
                    track = item["track"].get<int>();
                if (item.contains("year") && item["year"].is_number_integer())
                    year = item["year"].get<int>();
                if (item.contains("coverArt") && item["coverArt"].is_string())
                    coverArt = item["coverArt"].get<std::string>();                
                Song asong = Song(id, artist, title, album, url);
                asong.setDuration(duration);
                asong.setTrack(track);
                asong.setYear(year);
                asong.setCoverart(coverArt);
                queue_sid = SQ.addSong(asong);
                if((queue_sid != -1) && (queue_sid_t == -1))//记住第一个加入queue的id，作为返回值
                    queue_sid_t = queue_sid;
            }
        }
        //根据当前queue补全song position信息
        SQ.makeupPos();
        }else
            std::cerr << "jsonArray not is_array: " <<std::endl;   
    return  queue_sid_t;
}
//根据post的track，删除该track在queue中
void rmTrackBySid(struct mg_http_message *hm){
    json j;
    int queue_sid;
    try {
        // 解析 JSON 字符串
        std::string json_str = std::string(hm->body.buf,(int) hm->body.len);
        j = json::parse(json_str);
        queue_sid = j["queue_sid"].get<int>();
        SQ.removeSongBySid(queue_sid);
    } catch (const json::parse_error& e){
        // 处理解析错误
        std::cerr << "JSON parsing error: " << e.what() << std::endl; 
    }
}

void callback_http(struct mg_connection *c,struct mg_http_message *hm,const char *s_root_dir)
{
    int queue_sid;
    if (mg_match(hm->uri, mg_str("/api/hello"), NULL)) {              // REST API call?
        mg_http_reply(c, 200, "", "{%m:%d}\n", MG_ESC("status"), 1);    // Yes. Respond JSON
    }else if(mg_match(hm->uri, mg_str("/api/queue/add/*"), NULL)) {//将浏览器发送的json转成song并加入mpd queue
        if(mg_match(hm->method, mg_str("POST"),NULL)){
            //printf("POST request body: %.*s\n", (int) hm->body.len, hm->body.buf);
             queue_sid = addTrackTompd(hm);
            if(mg_match(hm->uri, mg_str("/api/queue/add/play"), NULL))
               SQ.playSId(queue_sid);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", SQ.toJson().c_str());
        }
    }else if(mg_match(hm->uri, mg_str("/api/queue/replace/*"), NULL)) {//将浏览器发送的json转成song并替换mpd queue
        if(mg_match(hm->method, mg_str("POST"),NULL)){
            //printf("POST request body: %.*s\n", (int) hm->body.len, hm->body.buf);
            SQ.clearQueue();
            addTrackTompd(hm);
            if(mg_match(hm->uri, mg_str("/api/queue/replace/play"), NULL))
                SQ.playPos(0);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", SQ.toJson().c_str());
        }
    }else if(mg_match(hm->uri, mg_str("/api/queue/del"), NULL)) { //删除某歌曲
        if(mg_match(hm->method, mg_str("POST"),NULL)){
            //printf("POST request body: %.*s\n", (int) hm->body.len, hm->body.buf);
            rmTrackBySid(hm);
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", SQ.toJson().c_str());
        }                      
    }else if(mg_match(hm->uri, mg_str("/api/queue"), NULL)) { //查询队列
        if(mg_match(hm->method, mg_str("GET"),NULL)){
            mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s", SQ.toJson().c_str());
        }
    }else{
        struct mg_http_serve_opts opts = {.root_dir = s_root_dir};  // For all other URLs,
        mg_http_serve_dir(c, hm, &opts);                     // Serve static files
    }
    //mg_send_status(c, 404);
    //mg_printf_data(c, "Not Found");
}