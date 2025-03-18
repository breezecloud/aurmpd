#include <iostream>
#include "json.hpp"

// 使用 nlohmann/json 命名空间
using json = nlohmann::json;

int main() {
    // 给定的 JSON 数组字符串
    
    std::string json_str = "[{\"id\":55,\"pos\":0,\"duration\":0,\"title\":\"stream.view?u=guest&t=9552e8369966cee81597c49bc6e5f640&s=0410989397215342&v=1.13.0&c=Aurial&f=json&id=7c0924c2522945c0c0827e3a19ef9bd1&format=mp3&maxBitRate=0&\",\"artist\":\"\",\"album\":\"\"}]";

    // 解析 JSON 字符串
    json json_data = json::parse(json_str);

    // 检查解析后的结果是否为数组
    if (json_data.is_array()) {
        std::cout << "array" << std::endl;

        // 遍历数组并访问其中的元素
        for (const auto& item : json_data) {
            int id = item["id"];
            int pos = item["pos"];
            int duration = item["duration"];
            std::string title = item["title"];
            std::string artist = item["artist"];
            std::string album = item["album"];

            // 注意：这里的 title 字段包含了很多参数，可能需要进一步处理
            // 输出元素的内容
            std::cout << "ID: " << id << ", Position: " << pos << ", Duration: " << duration
                      << ", Title: " << title << ", Artist: " << artist << ", Album: " << album << std::endl;
        }
    } else {
        std::cerr << "not array" << std::endl;
    }

    return 0;
}
