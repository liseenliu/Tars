#include "wifi_board.h"
#include "led.h"
#include "config.h"
#include "display/ssd1306_display.h"

#include <esp_log.h>
#include <driver/i2c_master.h>
#define TAG "ESP32S3Board"

class ESP32S3Board : public WifiBoard
{
private:
    i2c_master_bus_handle_t display_i2c_bus_;
    void InitializeDisplayI2c()
    {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
    }
public:
    
    ESP32S3Board() 
    {
        InitializeDisplayI2c();
    }
    

    virtual Led* GetBuiltinLed() override
    {
        static Led led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual Display* GetDisplay() override
    {
        static SSD1306Display display(display_i2c_bus_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
        return &display;
    }
};

DECLARE_BOARD(ESP32S3Board);
