#include "websocket_protocol.h"
#include "board.h"
#include "system_info.h"
#include "application.h"

#include <cstring>
#include <esp_log.h>
#include <cJSON.h>
#include <arpa/inet.h>

#define TAG "WebsocketProtocol"

WebsocketProtocol::WebsocketProtocol()
{
    event_group_handle_ = xEventGroupCreate();
}

WebsocketProtocol::~WebsocketProtocol() {
    vEventGroupDelete(event_group_handle_);
}

void WebsocketProtocol::SendAudio(const std::vector<uint8_t>& data) {

}


