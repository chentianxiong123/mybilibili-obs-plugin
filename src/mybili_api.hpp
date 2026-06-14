#pragma once
#include <string>
#include <vector>
#include "json11/json11.hpp"

namespace MyBili {

struct Config {
    // Auth
    std::string token;
    std::string refreshToken;

    // Room
    int roomId = 0;
    int userId = 0;
    std::string roomName;
    std::string streamKey;
    std::string rtmpAddr;

    // State
    bool loginStatus = false;
    bool streaming = false;

    // Backend API base URL (without trailing slash)
    std::string apiBaseUrl = "http://localhost:80/api";
};

class MbgApi {
public:
    static void init();
    static void cleanup();

    // QR auth
    static bool generateQr(std::string &qrId, std::string &qrUrl, std::string &message);
    static bool pollQrStatus(const std::string &qrId, Config &config, std::string &message);

    // Room
    static bool getMyRoom(Config &config, std::string &message);
    static bool startLive(Config &config, std::string &message);
    static bool stopLive(Config &config, std::string &message);
    static bool updateRoomInfo(Config &config, const std::string &newRoomName, std::string &message);

private:
    static std::vector<std::string> buildJsonHeaders(const std::string &token);
    static bool parseRoomResponse(const json11::Json &json, Config &config);
};

} // namespace MyBili