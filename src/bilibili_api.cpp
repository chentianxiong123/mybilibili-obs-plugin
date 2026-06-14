#include "bilibili_api.hpp"
#include "http_client.hpp"
#include "plugin_utils.hpp"
#include "util/base.h"
#include <sstream>

// ============================================================
// This file keeps the exact same function signatures as the
// original obs-bilibili-stream, but replaces every API call
// with calls to the mybilibili backend (localhost:8080).
//
// Build system stays unchanged — the DLL loads in OBS.
// ============================================================

namespace Bili {

static const std::vector<std::string> default_headers = {
    "Accept: application/json, text/plain, */*",
    "Accept-Language: zh-CN,zh;q=0.9",
    "Content-Type: application/json; charset=UTF-8",
    "User-Agent: mybilibili-obs-plugin/2.1.0"};

static const long REQUEST_TIMEOUT_MS = 10000;

// ── Backend URL ─────────────────────────────────────────
// Reads MYBILIBILI_API_BASE from environment, falls back to
// http://localhost:8080 if not set.
static std::string getApiBase()
{
    const char *env = std::getenv("MYBILIBILI_API_BASE");
    if (env && env[0] != '\0')
        return std::string(env);
    return "http://localhost:8080";
}

// ── Helper ──────────────────────────────────────────────────
static std::string baseUrl(const std::string &path)
{
    return getApiBase() + path;
}

std::vector<std::string> BiliApi::buildHeaders(const std::string &token)
{
    auto headers = default_headers;
    if (!token.empty()) {
        headers.push_back("Authorization: Bearer " + token);
    }
    return headers;
}

void BiliApi::init()
{
    Http::HttpClient::init();
}

void BiliApi::cleanup()
{
    Http::HttpClient::cleanup();
}

// ── QR generate ─────────────────────────────────────────────
bool BiliApi::getQrCode(const std::string &cookies, std::string &qr_data, std::string &qr_key,
                         std::string &message)
{
    auto headers = buildHeaders(cookies);
    auto response = Http::HttpClient::post(
        baseUrl("/api/user/qr/generate"), "{}", headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "获取二维码失败，状态码: " + std::to_string(response.status);
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    auto data = json["data"];
    qr_data = data["qrUrl"].string_value();   // URL for QR image
    qr_key = data["qrId"].string_value();      // ID to poll
    if (qr_data.empty() || qr_key.empty()) {
        message = "二维码数据不完整";
        return false;
    }

    return true;
}

// ── QR poll ─────────────────────────────────────────────────
bool BiliApi::qrLogin(std::string &qr_key, std::string &cookies, std::string &message)
{
    std::string body = "{\"qrId\":\"" + qr_key + "\"}";
    auto response = Http::HttpClient::post(
        baseUrl("/api/user/qr/status"), body, default_headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "查询状态失败，状态码: " + std::to_string(response.status);
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty()) {
        message = "JSON解析失败: " + err;
        return false;
    }

    std::string status = json["data"]["status"].string_value();
    if (status == "confirmed") {
        cookies = json["data"]["token"].string_value(); // store token in cookies field
        return true;
    }
    if (status == "expired") {
        message = "二维码已过期";
        return false;
    }
    message = "等待扫码...";
    return false;
}

// ── Check login ─────────────────────────────────────────────
bool BiliApi::checkLoginStatus(const std::string &cookies, std::string &message, std::string &mid)
{
    auto headers = buildHeaders(cookies);
    auto response = Http::HttpClient::get(
        baseUrl("/api/live/room/my"), headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "未登录";
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty() || json["code"].int_value() != 200) {
        message = "未登录";
        return false;
    }
    message = "已登录";
    return true;
}

// ── Get room ────────────────────────────────────────────────
bool BiliApi::getRoomIdAndCsrf(const std::string &cookies, std::string &room_id,
                                std::string &csrf_token, std::string &message)
{
    auto headers = buildHeaders(cookies);
    auto response = Http::HttpClient::get(
        baseUrl("/api/live/room/my"), headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "获取直播间失败";
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    auto data = json["data"];
    room_id = std::to_string(data["id"].int_value());
    csrf_token = "mybilibili";
    return true;
}

// ── Partition list (not used) ───────────────────────────────
json11::Json BiliApi::getPartitionList(std::string &message)
{
    return json11::Json();
}

// ── Start live ──────────────────────────────────────────────
bool BiliApi::startLive(Config &config, std::string &rtmp_addr, std::string &rtmp_code,
                         std::string &message, std::string &face_qr, std::string &mid)
{
    if (config.room_id.empty()) {
        message = "没有直播间信息";
        return false;
    }

    std::string url = baseUrl("/api/live/room/") + config.room_id + "/status";
    std::string body = "{\"status\":\"live\"}";
    auto headers = buildHeaders(config.cookies);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "开播失败，状态码: " + std::to_string(response.status);
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty() || json["code"].int_value() != 200) {
        message = "开播失败";
        return false;
    }

    rtmp_addr = config.rtmp_addr.empty()
                    ? "rtmp://localhost/live"
                    : config.rtmp_addr;
    rtmp_code = config.rtmp_code.empty()
                    ? config.room_id
                    : config.rtmp_code;
    return true;
}

// ── Stop live ───────────────────────────────────────────────
bool BiliApi::stopLive(const Config &config, std::string &message)
{
    if (config.room_id.empty()) {
        message = "没有直播间信息";
        return false;
    }

    std::string url = baseUrl("/api/live/room/") + config.room_id + "/status";
    std::string body = "{\"status\":\"offline\"}";
    auto headers = buildHeaders(config.cookies);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "下播失败";
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty() || json["code"].int_value() != 200) {
        message = "下播失败";
        return false;
    }
    return true;
}

// ── Update room info ────────────────────────────────────────
bool BiliApi::updateRoomInfo(const Config &config, const std::string &title, std::string &message)
{
    if (config.room_id.empty()) {
        message = "没有直播间信息";
        return false;
    }

    std::string url = baseUrl("/api/live/room/") + config.room_id;
    std::string body = "{\"roomName\":\"" + title + "\"}";
    auto headers = buildHeaders(config.cookies);
    auto response = Http::HttpClient::put(url, body, headers, REQUEST_TIMEOUT_MS);
    if (response.status != 200) {
        message = "更新失败";
        return false;
    }

    std::string err;
    json11::Json json = json11::Json::parse(response.data, err);
    if (!err.empty() || json["code"].int_value() != 200) {
        message = "更新失败";
        return false;
    }
    return true;
}

} // namespace Bili
