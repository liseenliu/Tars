#include "wifi_board.h"
#include "application.h"
#include "display/display.h"
#include "system_info.h"
#include "wifi_connect/wifi_station.h"
#include "wifi_connect/wifi_configuration_ap.h"

#define TAG "WifiBoard"

void WifiBoard::StartNetwork()
{
    auto& application = Application::GetInstance();
    auto display = Board::GetInstance().GetDisplay();
    auto builtin_led = Board::GetInstance().GetBuiltinLed();

    // Try to connect to WiFi, if failed, launch the WiFi configuration AP
    auto& wifi_station = WifiStation::GetInstance();
    display->SetStatus(std::string("正在连接 ") + wifi_station.GetSsid());
    wifi_station.Start();
    if (!wifi_station.IsConnected()) {
        builtin_led->SetBlue();
        builtin_led->Blink(1000, 500);
        auto& wifi_ap = WifiConfigurationAp::GetInstance();
        wifi_ap.SetSsidPrefix("Xiaozhi");
        wifi_ap.Start();
        
        // 播报配置 WiFi 的提示
        application.Alert("Info", "Configuring WiFi");

        // 显示 WiFi 配置 AP 的 SSID 和 Web 服务器 URL
        std::string hint = "请在手机上连接热点 ";
        hint += wifi_ap.GetSsid();
        hint += "，然后打开浏览器访问 ";
        hint += wifi_ap.GetWebServerUrl();

        display->SetStatus(hint);
        
        // Wait forever until reset after configuration
        while (true) {
            int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
            int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
            ESP_LOGI(TAG, "Free internal: %u minimal internal: %u", free_sram, min_free_sram);
            vTaskDelay(pdMS_TO_TICKS(10000));
        }
    }
}

/**
 * 获取wifit板子的json信息
 */
std::string WifiBoard::GetBoardJson()
{
    // Set the board type for OTA
    auto& wifi_station = WifiStation::GetInstance();
    std::string board_type = BOARD_TYPE;
    std::string board_json = std::string("{\"type\":\"" + board_type + "\",");
    if (!wifi_config_mode_) {
        board_json += "\"ssid\":\"" + wifi_station.GetSsid() + "\",";
        board_json += "\"rssi\":" + std::to_string(wifi_station.GetRssi()) + ",";
        board_json += "\"channel\":" + std::to_string(wifi_station.GetChannel()) + ",";
        board_json += "\"ip\":\"" + wifi_station.GetIpAddress() + "\",";
    }
    board_json += "\"mac\":\"" + SystemInfo::GetMacAddress() + "\"}";
    return board_json;
}