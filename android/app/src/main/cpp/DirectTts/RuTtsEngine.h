#pragma once
#include "DirectTtsEngine.h"
#include "AaudioPlayer.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class RuTtsEngine : public DirectTtsEngine {
public:
    RuTtsEngine();
    ~RuTtsEngine() override;

    void speak(std::string_view text, bool interrupt) override;
    void stop() override;

private:
    struct SynthTask {
        std::string koi8Text;
    };

    AaudioPlayer          m_player;
    std::atomic<bool>     m_stopCurrent{false};
    std::atomic<bool>     m_shutdown{false};

    std::mutex            m_queueMutex;
    std::condition_variable m_queueCv;
    std::queue<SynthTask> m_queue;
    std::thread           m_thread;

    void synthLoop();
    void synthesize(const std::string& koi8Text);

    static std::string utf8ToKoi8r(std::string_view utf8);
    static int audioCallback(void* buffer, size_t size, void* userData);
};
