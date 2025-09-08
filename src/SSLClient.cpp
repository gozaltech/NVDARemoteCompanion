#include "SSLClient.h"
#include "Debug.h"
#include "Config.h"
#include <iostream>
#include <cstring>

namespace {
    void LogSSLError(const std::string& operation, int ret) {
        char error_buf[Config::SSL_ERROR_BUFFER_SIZE];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        std::cerr << operation << ": " << error_buf << std::endl;
        DEBUG_ERROR("SSL", operation + ": " + std::string(error_buf));
    }
}

SSLClient::SSLClient() {
    mbedtls_net_init(&m_net_ctx);
    mbedtls_ssl_init(&m_ssl_ctx);
    mbedtls_ssl_config_init(&m_ssl_conf);
    mbedtls_entropy_init(&m_entropy);
    mbedtls_ctr_drbg_init(&m_ctr_drbg);
}

SSLClient::~SSLClient() {
    Disconnect();
}

bool SSLClient::InitializeSSL() {
    const char* pers = "ssl_client";
    int ret = mbedtls_ctr_drbg_seed(&m_ctr_drbg, mbedtls_entropy_func, &m_entropy,
                                    (const unsigned char*)pers, strlen(pers));
    if (ret != 0) {
        std::cerr << "Failed to seed RNG: " << ret << std::endl;
        return false;
    }

    ret = mbedtls_ssl_config_defaults(&m_ssl_conf,
                                      MBEDTLS_SSL_IS_CLIENT,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        std::cerr << "Failed to set SSL config defaults: " << ret << std::endl;
        return false;
    }

    mbedtls_ssl_conf_authmode(&m_ssl_conf, MBEDTLS_SSL_VERIFY_NONE);
    mbedtls_ssl_conf_rng(&m_ssl_conf, mbedtls_ctr_drbg_random, &m_ctr_drbg);

    ret = mbedtls_ssl_setup(&m_ssl_ctx, &m_ssl_conf);
    if (ret != 0) {
        std::cerr << "Failed to setup SSL context: " << ret << std::endl;
        return false;
    }

    ret = mbedtls_ssl_set_hostname(&m_ssl_ctx, m_serverName.c_str());
    if (ret != 0) {
        std::cerr << "Failed to set hostname: " << ret << std::endl;
        return false;
    }

    mbedtls_ssl_set_bio(&m_ssl_ctx, &m_net_ctx, mbedtls_net_send, mbedtls_net_recv, nullptr);

    return true;
}

void SSLClient::CleanupSSL() {
    mbedtls_ssl_free(&m_ssl_ctx);
    mbedtls_ssl_config_free(&m_ssl_conf);
    mbedtls_ctr_drbg_free(&m_ctr_drbg);
    mbedtls_entropy_free(&m_entropy);
    mbedtls_net_free(&m_net_ctx);
}

bool SSLClient::Connect(const std::string& host, int port) {
    if (!m_connectionState.AttemptTransition(ConnectionState::Status::Disconnected, ConnectionState::Status::Connecting)) {
        return false;
    }
    
    m_serverName = host;
    
    int ret = mbedtls_net_connect(&m_net_ctx, host.c_str(), std::to_string(port).c_str(),
                                  MBEDTLS_NET_PROTO_TCP);
    if (ret != 0) {
        LogSSLError("Failed to connect TCP", ret);
        m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
        return false;
    }

    DEBUG_INFO_F("SSL", "TCP connection established to {}:{}", host, port);

    if (!InitializeSSL()) {
        mbedtls_net_free(&m_net_ctx);
        m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
        return false;
    }

    while ((ret = mbedtls_ssl_handshake(&m_ssl_ctx)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            LogSSLError("SSL handshake failed", ret);
            CleanupSSL();
            m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
            return false;
        }
    }

    DEBUG_INFO("SSL", "SSL handshake completed successfully");
    m_connectionState.TransitionTo(ConnectionState::Status::Connected);
    return true;
}

void SSLClient::Disconnect() {
    DEBUG_VERBOSE("SSL", "Starting SSL disconnect");
    m_connectionState.TransitionTo(ConnectionState::Status::Disconnecting);
    
    if (m_net_ctx.fd != -1) {
        DEBUG_VERBOSE("SSL", "Sending close notify");
        mbedtls_ssl_close_notify(&m_ssl_ctx);
    }
    
    DEBUG_VERBOSE("SSL", "Cleaning up SSL resources");
    CleanupSSL();
    m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
    DEBUG_VERBOSE("SSL", "SSL disconnect completed");
}

bool SSLClient::IsConnected() const {
    return m_connectionState.IsConnected() && m_net_ctx.fd != -1;
}

int SSLClient::Send(const char* data, int length) {
    if (!IsConnected()) {
        return -1;
    }

    int ret = mbedtls_ssl_write(&m_ssl_ctx, (const unsigned char*)data, length);
    if (ret < 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
        }
        return -1;
    }

    return ret;
}

int SSLClient::Receive(char* buffer, int bufferSize) {
    if (!IsConnected()) {
        return -1;
    }

    int ret = mbedtls_ssl_read(&m_ssl_ctx, (unsigned char*)buffer, bufferSize);
    if (ret < 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            return 0;
        } else {
            m_connectionState.TransitionTo(ConnectionState::Status::Disconnected);
            return -1;
        }
    }

    return ret;
}