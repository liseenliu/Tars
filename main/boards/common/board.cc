#include "board.h"
#include "system_info.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_chip_info.h>
#include <string>

#define TAG "Board"

Board::Board()
{
}

std::string Board::GetJson()
{
    std::string json = "{";
    json += "\"flash_size\": " + std::to_string(SystemInfo::GetFlashSize()) + ",";
    json += "\"minimum_heap_size\": " + std::to_string(SystemInfo::GetMinimumHeapSize()) + ",";
    json += "\"free_heap_size\": " + std::to_string(SystemInfo::GetFreeHeapSize()) + ",";
    json += "\"mac_address\": \"" + SystemInfo::GetMacAddress() + "\",";
    json += "\"chip_model_name\": \"" + SystemInfo::GetChipModelName() + "\"";
    json += "},";

    json += "\"board\":" + GetBoardJson();
    json += "}";
    
    return json;
}
