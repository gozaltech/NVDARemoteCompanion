#pragma once
#include <string>
#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/error.h>
#include "ConnectionState.h"

class SSLClient {
private:
    mbedtls_net_context m_net_ctx;
    mbedtls_ssl_context m_ssl_ctx;
    mbedtls_ssl_config m_ssl_conf;
    mbedtls_entropy_context m_entropy;
    mbedtls_ctr_drbg_context m_ctr_drbg;
    std::string m_serverName;
    ConnectionState::StateManager m_connectionState;

public:
    SSLClient();
    ~SSLClient();
    
    bool Connect(const std::string& host, int port);
    void Disconnect();
    bool IsConnected() const;
    
    int Send(const char* data, int length);
    int Receive(char* buffer, int bufferSize);
    
private:
    bool InitializeSSL();
    void CleanupSSL();
};