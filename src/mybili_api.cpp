#include "mybili_api.hpp"
#include "http_client.hpp"
#include "plugin_utils.hpp"
#include "util/base.h"
#include <sstream>

namespace MyBili {

static const long REQUEST_TIMEOUT_MS = 10000;

static std::vector<std::string> defaultHeaders() {
    return {
        "Accept: application/json, text/plain, */*",
        "Accept-Language: zh-CN,zh;q=0.9",
        "Content-Type: application/json; charset=UTF-8",
        "User-Agent: mybilibili-obs-plugin/1.0",
        "X-Client-Platform: obs-plugin"
    };
}

std::vector<std::string> MbgApi::buildJsonHeaders(const std::string &token) {
    auto headers = defaultHeaders();
    if (!token.empty()) {
        headers.push_back("Authorization: Bearer " + token);
    }
    return headers;
}

void MbgApi::init() {
    Http::HttpClient::init();
}

void MbgApi::cleanup() {
    Http::HttpClient::cleanup();
}

bool MbgApi::generateQr(std::string &qrId, std::string &qrUrl, std::string &message) {
    auto headers = defaultHeaders();
    auto response = Http::HttpClient::post(
        "http://localhost:80/api/user/qr/generate",
        "", // empty body
        headers,
        REQUEST_TIMEOUT_MS
    );

    if (response.status != 200) {
        message = "请求失败，状态码: " + std::to_string(response.status);
        if (!response.data.empty()) message += ", " + response.data;
        obs_log(LOG_ERROR, "generateQr failed: %s", message.c_str());
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        obs_log(LOG_ERROR, "generateQr parse error: %s", err.c_str());
        return false;
    }

    int code = json["code"].int_value();
    if (code != 200) {
        message = json["message"].string_value();
        if (message.empty()) message = "生成二维码失败(code=" + std::to_string(code) + ")";
        obs_log(LOG_ERROR, "generateQr api error: %s", message.c_str());
        return false;
    }

    qrId = json["data"]["qrId"].string_value();
    qrUrl = json["data"]["qrUrl"].string_value();

    if (qrId.empty() || qrUrl.empty()) {
        message = "返回数据缺少qrId或qrUrl";
        return false;
    }

    obs_log(LOG_INFO, "QR generated: qrId=%s", qrId.c_str());
    return true;
}

bool MbgApi::pollQrStatus(const std::string &qrId, Config &config, std::string &message) {
    // POST /api/user/qr/status with body { "qrId": "..." }
    std::string body = "{\"qrId\":\"" + qrId + "\"}";
    auto headers = defaultHeaders();
    auto response = Http::HttpClient::post(
        "http://localhost:80/api/user/qr/status",
        body,
        headers,
        REQUEST_TIMEOUT_MS
    );

    if (response.status != 200) {
        message = "状态查询失败，状态码: " + std::to_string(response.status);
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    std::string status = json["data"]["status"].string_value();
    obs_log(LOG_INFO, "QR status poll: %s", status.c_str());

    if (status == "confirmed") {
        config.token = json["data"]["token"].string_value();
        config.refreshToken = json["data"]["refreshToken"].string_value();
        message = "扫码成功";
        return true;
    }

    if (status == "expired") {
        message = "二维码已过期";
        return false;
    }

    // "pending" — not yet scanned
    message = "等待扫码...";
    return false;
}

bool MbgApi::getMyRoom(Config &config, std::string &message) {
    if (config.token.empty()) {
        message = "未登录";
        return false;
    }

    auto headers = buildJsonHeaders(config.token);
    auto response = Http::HttpClient::get(
        "http://localhost:80/api/live/room/my",
        headers,
        REQUEST_TIMEOUT_MS
    );

    if (response.status != 200) {
        message = "获取直播间失败，状态码: " + std::to_string(response.status);
        obs_log(LOG_ERROR, "getMyRoom failed: %s", message.c_str());
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    int code = json["code"].int_value();
    if (code != 200) {
        message = json["message"].string_value();
        if (message.empty()) message = "获取直播间失败(code=" + std::to_string(code) + ")";
        return false;
    }

    if (json["data"].is_null()) {
        message = "暂无直播间，请在Web端创建";
        return false;
    }

    return parseRoomResponse(json, config);
}

bool MbgApi::parseRoomResponse(const json11::Json &json, Config &config) {
    auto data = json["data"];
    config.roomId = data["id"].int_value();
    config.roomName = data["roomName"].string_value();
    config.streamKey = data["streamKey"].string_value();
    config.rtmpAddr = "rtmp://localhost/live";

    if (config.roomId <= 0 || config.streamKey.empty()) {
        return false;
    }
    return true;
}

bool MbgApi::startLive(Config &config, std::string &message) {
    if (config.token.empty() || config.roomId <= 0) {
        message = "未登录或没有直播间";
        return false;
    }

    std::string url = "http://localhost:80/api/live/room/" + std::to_string(config.roomId) + "/status";
    std::string body = "{\"status\":\"live\"}";
    auto headers = buildJsonHeaders(config.token);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);

    if (response.status != 200) {
        message = "开播失败，状态码: " + std::to_string(response.status);
        obs_log(LOG_ERROR, "startLive failed: %s", response.data.c_str());
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    int code = json["code"].int_value();
    if (code != 200) {
        message = json["message"].string_value();
        if (message.empty()) message = "开播失败(code=" + std::to_string(code) + ")";
        return false;
    }

    obs_log(LOG_INFO, "Live started: roomId=%d, streamKey=%s", config.roomId, config.streamKey.c_str());
    return true;
}

bool MbgApi::stopLive(Config &config, std::string &message) {
    if (config.token.empty() || config.roomId <= 0) {
        message = "未登录或没有直播间";
        return false;
    }

    std::string url = "http://localhost:80/api/live/room/" + std::to_string(config.roomId) + "/status";
    std::string body = "{\"status\":\"offline\"}";
    auto headers = buildJsonHeaders(config.token);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);

    if (response.status != 200) {
        message = "下播失败，状态码: " + std::to_string(response.status);
        obs_log(LOG_ERROR, "stopLive failed: %s", response.data.c_str());
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    int code = json["code"].int_value();
    if (code != 200) {
        message = json["message"].string_value();
        if (message.empty()) message = "下播失败(code=" + std::to_string(code) + ")";
        return false;
    }

    obs_log(LOG_INFO, "Live stopped: roomId=%d", config.roomId);
    return true;
}

bool MbgApi::updateRoomInfo(Config &config, const std::string &newRoomName, std::string &message) {
    if (config.token.empty() || config.roomId <= 0) {
        message = "未登录或没有直播间";
        return false;
    }

    std::string url = "http://localhost:80/api/live/room/" + std::to_string(config.roomId);
    std::string body = "{\"roomName\":\"" + json11::Json(newRoomName).string_value() + "\"}";
    auto headers = buildJsonHeaders(config.token);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);

    if (response.status != 200) {
        message = "更新失败，状态码: " + std::to_string(response.status);
        obs_log(LOG_ERROR, "updateRoomInfo failed: %s", response.data.c_str());
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    int code = json["code"].int_value();
    if (code != 200) {
        message = json["message"].string_value();
        if (message.empty()) message = "更新失败(code=" + std::to_string(code) + ")";
        return false;
    }

    config.roomName = newRoomName;
    obs_log(LOG_INFO, "Room info updated: roomName=%s", newRoomName.c_str());
    return true;
}

} // namespace MyBili