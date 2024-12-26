#ifndef _SYSTEM_INFO_H_
#define _SYSTEM_INFO_H_

#include <string>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>

class SystemInfo
{
public:
    static size_t GetFlashSize();
    static size_t GetMinimumHeapSize();
    static size_t GetFreeHeapSize();
    static std::string GetMacAddress();
    static std::string GetChipModelName();
};

#endif // _SYSTEM_INFO_H_