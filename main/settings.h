#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <string>
#include <nvs_flash.h>

class Settings
{
public:
    Settings(const std::string& ns, bool read_write = false);
    ~Settings();
    
    std::string GetString(const std::string& key, const std::string& default_value = "");
    void SetString(const std::string& key, const std::string& value);
    int32_t GetInt(const std::string& key, int32_t default_value = 0);
    void SetInt(const std::string& key, int32_t value);
    void EraseKey(const std::string& key);
    void EraseAll();

private:
    std::string ns_;
    nvs_handle_t nvs_handle_ = 0;
    bool read_write_ = false;
    bool dirty_ = false;
};

#endif // _SETTINGS_H_