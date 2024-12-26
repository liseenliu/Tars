#include "wifi_board.h"

/**
 * 获取wifit板子的json信息
 */
std::string WifiBoard::GetBoardJson()
{
    std::string json = "\"wifi_config_mode\": " + std::to_string(wifi_config_mode_);
    return json;
}