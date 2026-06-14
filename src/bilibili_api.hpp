#pragma once
#include <string>
#include <vector>
#include "json11/json11.hpp"
#include "plugin_utils.hpp"

namespace Bili {

struct Config {
    // Auth (replaces cookies)
    std::string token;
    std::string refreshToken;

    // Room
    std::string room_id;       // kept as string for compat
    std::string csrf_token;    // kept for compat (not used by mybilibili)
    std::string cookies;       // kept for compat (not used by mybilibili)
    std::string mid;           // kept for compat
    std::string title;
    bool login_status = false;
    bool streaming = false;
    std::string rtmp_addr;
    std::string rtmp_code;
    int part_id = 2;
    int area_id = 86;
};

class BiliApi {
public:
    static void init();
    static void cleanup();

    // QR login — changed to mybilibili backend
    static bool getQrCode(const std::string &cookies, std::string &qr_data, std::string &qr_key,
                          std::string &message);
    static bool qrLogin(std::string &qr_key, std::string &cookies, std::string &message);
    static bool checkLoginStatus(const std::string &cookies, std::string &message, std::string &mid);
    static bool getRoomIdAndCsrf(const std::string &cookies, std::string &room_id,
                                 std::string &csrf_token, std::string &message);
    static json11::Json getPartitionList(std::string &message);

    // Stream control — changed to mybilibili backend
    static bool startLive(Config &config, std::string &rtmp_addr, std::string &rtmp_code,
                          std::string &message, std::string &face_qr, std::string &mid);
    static bool stopLive(const Config &config, std::string &message);
    static bool updateRoomInfo(const Config &config, const std::string &title, std::string &message);

private:
    static std::vector<std::string> buildHeaders(const std::string &token_or_cookies);
};

} // namespace Bili
