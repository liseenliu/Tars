#ifndef _Board_H_
#define _Board_H_

#include <string>
#include "led.h"
#include "web_socket/web_socket.h"
#include "http/http.h"

void *create_board();
class Display;
class AudioCodec;
class Board
{
private:
    Board(const Board &) = delete;            // 禁用拷贝构造函数
    Board &operator=(const Board &) = delete; // 禁用赋值操作
    
protected:
    Board();
    virtual std::string GetBoardJson() = 0;
public:
    static Board &GetInstance()
    {
        static Board* instance = nullptr;
        if (nullptr == instance) {
            instance = static_cast<Board*>(create_board());
        }
        return *instance;
    }
    virtual ~Board() = default;
    virtual Led* GetBuiltinLed() = 0;
    virtual Display* GetDisplay() = 0;
    virtual AudioCodec* GetAudioCodec() = 0;
    virtual Http* CreateHttp() = 0;
    virtual void StartNetwork() = 0;
    virtual WebSocket* CreateWebSocket() = 0;
    virtual bool GetNetworkState(std::string& network_name, int& signal_quality, std::string& signal_quality_text) = 0;
    virtual const char* GetNetworkStateIcon() = 0;
    virtual std::string GetJson();
    virtual void SetPowerSaveMode(bool enabled) = 0;
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
void* create_board() { \
    return new BOARD_CLASS_NAME(); \
}

#endif // _Board_H_