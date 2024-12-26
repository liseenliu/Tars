#ifndef _Board_H_
#define _Board_H_

#include <string>
#include "led.h"

void *create_board();
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
    virtual std::string GetJson();
};

#define DECLARE_BOARD(BOARD_CLASS_NAME) \
void* create_board() { \
    return new BOARD_CLASS_NAME(); \
}

#endif // _Board_H_