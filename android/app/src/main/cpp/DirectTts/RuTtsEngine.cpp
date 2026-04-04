#include "RuTtsEngine.h"
#include "ru_tts.h"
#include <android/log.h>

#define TAG "NVDARemote/RuTTS"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static constexpr size_t WAVE_BUF_SIZE = 512;

static constexpr uint8_t koi8rTable[64] = {
    0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
    0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1,
    0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
    0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1,
};

std::string RuTtsEngine::utf8ToKoi8r(std::string_view utf8) {
    std::string out;
    out.reserve(utf8.size());

    for (size_t i = 0; i < utf8.size(); ) {
        uint8_t b0 = static_cast<uint8_t>(utf8[i]);

        if (b0 < 0x80) {
            out += static_cast<char>(b0);
            ++i;
        } else if (b0 == 0xD0 && i + 1 < utf8.size()) {
            uint8_t b1 = static_cast<uint8_t>(utf8[i + 1]);
            uint32_t cp = 0x0400 | (b1 & 0x3F);
            if (cp == 0x0401) {
                out += static_cast<char>(0xB3);
            } else if (cp >= 0x0410 && cp <= 0x043F) {
                out += static_cast<char>(koi8rTable[cp - 0x0410]);
            }
            i += 2;
        } else if (b0 == 0xD1 && i + 1 < utf8.size()) {
            uint8_t b1 = static_cast<uint8_t>(utf8[i + 1]);
            uint32_t cp = 0x0440 | (b1 & 0x3F);
            if (cp == 0x0451) {
                out += static_cast<char>(0xA3);
            } else if (cp >= 0x0440 && cp <= 0x044F) {
                out += static_cast<char>(koi8rTable[cp - 0x0410]);
            }
            i += 2;
        } else {
            if      ((b0 & 0xE0) == 0xC0) i += 2;
            else if ((b0 & 0xF0) == 0xE0) i += 3;
            else if ((b0 & 0xF8) == 0xF0) i += 4;
            else                          ++i;
        }
    }
    return out;
}

struct CallbackCtx {
    AaudioPlayer*     player;
    std::atomic<bool>* stopFlag;
};

int RuTtsEngine::audioCallback(void* buffer, size_t size, void* userData) {
    auto* ctx = static_cast<CallbackCtx*>(userData);
    if (ctx->stopFlag->load(std::memory_order_relaxed)) return 1;
    bool ok = ctx->player->write(static_cast<int8_t*>(buffer), static_cast<int32_t>(size));
    return (!ok || ctx->stopFlag->load(std::memory_order_relaxed)) ? 1 : 0;
}

RuTtsEngine::RuTtsEngine()
    : m_thread([this]{ synthLoop(); }) {
    if (m_player.open()) m_player.pause();
}

RuTtsEngine::~RuTtsEngine() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_shutdown.store(true);
        m_stopCurrent.store(true);
        while (!m_queue.empty()) m_queue.pop();
    }
    m_queueCv.notify_all();
    if (m_thread.joinable()) m_thread.join();
    m_player.close();
}

void RuTtsEngine::speak(std::string_view text, bool interrupt) {
    std::string koi8 = utf8ToKoi8r(text);
    if (koi8.empty()) return;

    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        if (interrupt) {
            while (!m_queue.empty()) m_queue.pop();
            m_stopCurrent.store(true, std::memory_order_relaxed);
        }
        m_queue.push({std::move(koi8)});
    }
    m_queueCv.notify_one();
}

void RuTtsEngine::stop() {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_queue.empty()) m_queue.pop();
        m_stopCurrent.store(true, std::memory_order_relaxed);
    }
    m_queueCv.notify_all();
}

void RuTtsEngine::synthLoop() {
    while (true) {
        SynthTask task;
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCv.wait(lock, [this]{
                return !m_queue.empty() || m_shutdown.load();
            });
            if (m_shutdown.load() && m_queue.empty()) break;
            task = std::move(m_queue.front());
            m_queue.pop();
        }

        m_stopCurrent.store(false, std::memory_order_relaxed);

        bool ready = m_player.isOpen() ? m_player.resume() : m_player.open();
        if (!ready) {
            LOGE("Failed to start AAudio player");
            continue;
        }

        synthesize(task.koi8Text);

        m_player.pause();
    }
}

void RuTtsEngine::synthesize(const std::string& koi8Text) {
    ru_tts_conf_t conf;
    ru_tts_config_init(&conf);

    CallbackCtx ctx{ &m_player, &m_stopCurrent };

    std::vector<char> waveBuf(WAVE_BUF_SIZE);
    ru_tts_transfer(&conf, koi8Text.c_str(),
                    waveBuf.data(), waveBuf.size(),
                    audioCallback, &ctx);
}
