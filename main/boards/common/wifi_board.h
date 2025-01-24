#ifndef _WIFI_BOARD_H_
#define _WIFI_BOARD_H_

#include "board.h"
#include "web_socket/web_socket.h"

class WifiBoard : public Board
{
protected:
    bool wifi_config_mode_ = false;
    std::string GetBoardJson() override;
public:
    virtual void StartNetwork() override;
    virtual WebSocket* CreateWebSocket() override;
    virtual Http* CreateHttp() override;
    virtual bool GetNetworkState(std::string& network_name, int& signal_quality, std::string& signal_quality_text) override;
    virtual const char* GetNetworkStateIcon() override;
    virtual void SetPowerSaveMode(bool enabled) override;
    virtual void ResetWifiConfiguration();
};

#endif // _WIFI_BOARD_H_