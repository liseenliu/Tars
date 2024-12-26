#include "application.h"
#include "board.h"

#define TAG "Application"

Application::Application()
{
    event_group_ = xEventGroupCreate();
}

Application::~Application()
{
    vEventGroupDelete(event_group_);
}

void Application::Start()
{
    auto &board = Board::GetInstance();
    auto builtin_led = board.GetBuiltinLed();
    builtin_led->SetBlue();
    builtin_led->StartContinuousBlink(1000);
}

void Application::Schedule(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    main_tasks_.push_back(std::move(callback));
    xEventGroupSetBits(event_group_, SCHEDULE_EVENT);
}

void Application::MainLoop()
{
    while (true)
    {
        auto bits = xEventGroupWaitBits(event_group_,
                                        SCHEDULE_EVENT | AUDIO_INPUT_READY_EVENT | AUDIO_OUTPUT_READY_EVENT,
                                        pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & AUDIO_INPUT_READY_EVENT)
        {
            InputAudio();
        }
        if (bits & AUDIO_OUTPUT_READY_EVENT)
        {
            OutputAudio();
        }
        if (bits & SCHEDULE_EVENT)
        {
            mutex_.lock();
            std::list<std::function<void()>> tasks = std::move(main_tasks_);
            mutex_.unlock();
            for (auto &task : tasks)
            {
                task();
            }
        }
    }
}

void Application::InputAudio()
{
}

void Application::OutputAudio()
{
}

void Application::ResetDecoder()
{
}

void Application::SetDecodeSampleRate(int sample_rate)
{
}

void Application::CheckNewVersion()
{
}