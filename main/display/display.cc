#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>

#include "display.h"

#define TAG "Display"

Display::Display(){
    // Notification timer
    esp_timer_create_args_t notification_timer_args = {
        .callback = [](void *arg) {
            Display *display = static_cast<Display*>(arg);
            DisplayLockGuard lock(display);
            lv_obj_add_flag(display->notification_label_, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(display->status_label_, LV_OBJ_FLAG_HIDDEN);
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "Notification Timer",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&notification_timer_args, &notification_timer_));

    // Update display timer
    esp_timer_create_args_t update_display_timer_args = {
        .callback = [](void *arg) {
            Display *display = static_cast<Display*>(arg);
            display->Update();
        },
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "Update Display Timer",
        .skip_unhandled_events = false,
    };
    ESP_ERROR_CHECK(esp_timer_create(&update_display_timer_args, &update_timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(update_timer_, 1000000));
}

Display::~Display() {
    esp_timer_stop(notification_timer_);
    esp_timer_stop(update_timer_);
    esp_timer_delete(notification_timer_);
    esp_timer_delete(update_timer_);

    if (network_label_ != nullptr) {
        lv_obj_del(network_label_);
        lv_obj_del(notification_label_);
        lv_obj_del(status_label_);
        lv_obj_del(mute_label_);
        lv_obj_del(battery_label_);
    }
}

void Display::SetStatus(const std::string &status) {
    if (status_label_ == nullptr) {
        return;
    }
    DisplayLockGuard lock(this);
    lv_label_set_text(status_label_, status.c_str());
}

void Display::ShowNotification(const std::string &notification, int duration_ms) {
    if (notification_label_ == nullptr) {
        return;
    }
    DisplayLockGuard lock(this);
    lv_label_set_text(notification_label_, notification.c_str());
    lv_obj_clear_flag(notification_label_, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status_label_, LV_OBJ_FLAG_HIDDEN);

    esp_timer_stop(notification_timer_);
    ESP_ERROR_CHECK(esp_timer_start_once(notification_timer_, duration_ms * 1000));
}

void Display::Update()
{
    if (mute_label_ == nullptr) {
        return;
    }

}

void Display::SetEmotion(const std::string &emotion) {
    if (emotion_label_ == nullptr) {
        return;
    }
    DisplayLockGuard lock(this);
}

void Display::SetIcon(const char* icon) {
    if (emotion_label_ == nullptr) {
        return;
    }
    DisplayLockGuard lock(this);
    lv_label_set_text(emotion_label_, icon);
}

void Display::SetChatMessage(const std::string &role, const std::string &content) {
}

