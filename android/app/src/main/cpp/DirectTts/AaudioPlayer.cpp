#include "AaudioPlayer.h"
#include <android/log.h>
#include <vector>

#define TAG "NVDARemote/AAudio"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

static constexpr int32_t SAMPLE_RATE      = 10000;
static constexpr int32_t CHANNEL_COUNT    = 1;
static constexpr int64_t WRITE_TIMEOUT_NS = 2000000000LL;
static constexpr int32_t PRIME_FRAMES     = 200;

AaudioPlayer::AaudioPlayer() = default;

AaudioPlayer::~AaudioPlayer() {
    close();
}

bool AaudioPlayer::open() {
    if (m_stream) return true;

    AAudioStreamBuilder* builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("createStreamBuilder failed: %s", AAudio_convertResultToText(result));
        return false;
    }

    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setSampleRate(builder, SAMPLE_RATE);
    AAudioStreamBuilder_setChannelCount(builder, CHANNEL_COUNT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
#if __ANDROID_API__ >= 28
    AAudioStreamBuilder_setUsage(builder, AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY);
    AAudioStreamBuilder_setContentType(builder, AAUDIO_CONTENT_TYPE_SPEECH);
#endif

    result = AAudioStreamBuilder_openStream(builder, &m_stream);
    AAudioStreamBuilder_delete(builder);

    if (result != AAUDIO_OK) {
        LOGE("openStream failed: %s", AAudio_convertResultToText(result));
        m_stream = nullptr;
        return false;
    }

    result = AAudioStream_requestStart(m_stream);
    if (result != AAUDIO_OK) {
        LOGE("requestStart failed: %s", AAudio_convertResultToText(result));
        AAudioStream_close(m_stream);
        m_stream = nullptr;
        return false;
    }

    LOGI("AAudio stream opened: %d Hz, mono, 16-bit", SAMPLE_RATE);

    std::vector<int16_t> silence(PRIME_FRAMES, 0);
    AAudioStream_write(m_stream, silence.data(), PRIME_FRAMES, WRITE_TIMEOUT_NS);

    return true;
}

bool AaudioPlayer::write(const int8_t* data, int32_t bytes) {
    if (!m_stream || bytes <= 0) return true;

    std::vector<int16_t> buf(bytes);
    for (int32_t i = 0; i < bytes; i++) {
        buf[i] = static_cast<int16_t>(data[i]) << 8;
    }

    const int16_t* ptr = buf.data();
    int32_t remaining = bytes;
    while (remaining > 0) {
        aaudio_result_t written = AAudioStream_write(
            m_stream, ptr, remaining, WRITE_TIMEOUT_NS);
        if (written < 0) {
            LOGE("AAudio write error: %s", AAudio_convertResultToText(written));
            close();
            return false;
        }
        ptr       += written;
        remaining -= written;
    }
    return true;
}

void AaudioPlayer::pause() {
    if (!m_stream) return;
    aaudio_result_t r = AAudioStream_requestPause(m_stream);
    if (r != AAUDIO_OK)
        LOGE("requestPause failed: %s", AAudio_convertResultToText(r));
}

bool AaudioPlayer::resume() {
    if (!m_stream) return open();

    aaudio_result_t r = AAudioStream_requestStart(m_stream);
    if (r != AAUDIO_OK) {
        LOGE("requestStart (resume) failed: %s", AAudio_convertResultToText(r));
        close();
        return open();
    }
    return true;
}

void AaudioPlayer::stop() {
    if (!m_stream) return;
    AAudioStream_requestStop(m_stream);
    close();
}

void AaudioPlayer::close() {
    if (m_stream) {
        AAudioStream_requestStop(m_stream);
        AAudioStream_close(m_stream);
        m_stream = nullptr;
    }
}
