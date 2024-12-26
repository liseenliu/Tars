#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>

#include <string>
#include <mutex>
#include <list>

#define SCHEDULE_EVENT (1 << 0)
#define AUDIO_INPUT_READY_EVENT (1 << 1)
#define AUDIO_OUTPUT_READY_EVENT (1 << 2)

class Application
{
public:
    static Application &GetInstance()
    {
        static Application instance;
        return instance;
    }
    // 删除拷贝构造函数和赋值运算符
    Application(const Application &) = delete;
    Application &operator=(const Application &) = delete;
    void Start();
    void Schedule(std::function<void()> callback);

private:
    std::mutex mutex_;
    std::list<std::function<void()>> main_tasks_;

    EventGroupHandle_t event_group_;
    Application();
    ~Application();
    void MainLoop();
    void InputAudio();
    void OutputAudio(); 
    void ResetDecoder();
    void SetDecodeSampleRate(int sample_rate);
    void CheckNewVersion();
};

#endif // _APPLICATION_H_