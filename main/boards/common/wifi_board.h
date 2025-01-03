#ifndef _WIFI_BOARD_H_
#define _WIFI_BOARD_H_

#include "board.h"
class WifiBoard : public Board
{
protected:
    bool wifi_config_mode_ = false;
    std::string GetBoardJson() override;
public:
    virtual void StartNetwork() override;
};

#endif // _WIFI_BOARD_H_