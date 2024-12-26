#include <esp_log.h>
#include <esp_err.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <driver/gpio.h>
#include "system_info.h"
#include "application.h"

#define TAG "main"
extern "C" void app_main()
{
    // Initialize the default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize NVS flash for WiFi configuration
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash to fix corruption");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    Application::GetInstance().Start();

    // Dump CPU usage every 10 second
    while (true)
    {   
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        int free_sram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        int min_free_sram = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
        std::string flash_size = std::to_string(SystemInfo::GetFlashSize());
        ESP_LOGI(TAG, "Free internal: %u minimal internal: %u FlashSize: %s", free_sram, min_free_sram, flash_size.c_str());
    }
}