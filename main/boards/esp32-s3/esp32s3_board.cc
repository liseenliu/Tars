#include "wifi_board.h"
#include "led.h"
#include "config.h"

#include <esp_log.h>

#define TAG "ESP32S3Board"

class ESP32S3Board : public WifiBoard
{
private:
    /* data */
public:
    virtual Led* GetBuiltinLed() override
    {
        static Led led(BUILTIN_LED_GPIO);
        return &led;
    }
   
};

DECLARE_BOARD(ESP32S3Board);
