
#ifndef VGATEWAY_SSLCONTEXT_H
#define VGATEWAY_SSLCONTEXT_H

#include <memory>
#include "openssl/tls1.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "config.h"
#include <oatpp/base/Log.hpp>


int passwordCB(char* buffer, int size, int rw_flag, void* user_data) {
    const std::string* pass = static_cast<std::string*>(user_data);
    if (pass->size() > static_cast<size_t>(size)) {
        return 0;
    }
    std::strncpy(buffer, pass->c_str(), size);
    return static_cast<int>(pass->length());
}


static void printStateCB(const SSL *ssl, int type, int val) {
    const char *str = SSL_state_string_long(ssl);
    std::string strType = "";
    if (type & SSL_CB_ALERT) {
        strType += "SSL_CB_ALERT";
    }
    if (type & SSL_CB_EXIT) {
        strType += " SSL_CB_EXIT";
    }
    if (type & SSL_CB_LOOP) {
        strType += " SSL_CB_LOOP";
    }
    if (type & SSL_CB_READ) {
        strType += " SSL_CB_READ";
    }
    if (type & SSL_CB_WRITE) {
        strType += " SSL_CB_WRITE";
    }
    if (type & SSL_CB_ACCEPT_EXIT) {
        strType += " SSL_CB_ACCEPT_EXIT";
    }
    if (type & SSL_CB_ACCEPT_LOOP) {
        strType += " SSL_CB_ACCEPT_LOOP";
    }
    if (type & SSL_CB_CONNECT_EXIT) {
        strType += " SSL_CB_CONNECT_EXIT";
    }
    if (type & SSL_CB_CONNECT_LOOP) {
        strType += " SSL_CB_CONNECT_LOOP";
    }
    if (type & SSL_CB_HANDSHAKE_DONE) {
        strType += " SSL_CB_HANDSHAKE_DONE";
    }
    if (type & SSL_CB_HANDSHAKE_START) {
        strType += " SSL_CB_HANDSHAKE_START";
    }
    if (type & SSL_CB_READ_ALERT) {
        strType += " SSL_CB_READ_ALERT";
    }
    if (type & SSL_CB_WRITE_ALERT) {
        strType += " SSL_CB_WRITE_ALERT";
    }
    OATPP_LOGi(__func__, "SSL state: {} and type = {} val = {}", str, strType, val)
    //printf("SSL state: %s and type = %s val = %d\n", str, strType.c_str(), val);
}

SSL_CTX* getSSLContext(const std::shared_ptr<Config> &m_cmdArgs) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    
    // manually create the context
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(__func__, "Failed to create SSL_CTX = {}", errMsg)
        exit(-1);
    }
    //set call back and phrase 
    SSL_CTX_set_default_passwd_cb(ctx, passwordCB);
    SSL_CTX_set_default_passwd_cb_userdata(ctx, &m_cmdArgs->server_phrase);
    
    //load keys and certificates
    if (SSL_CTX_use_certificate_file(ctx, m_cmdArgs->server_cert_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(__func__ , "Failed to load - {} file, err= {}", m_cmdArgs->server_cert_filename, errMsg)
        exit(-1);
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, m_cmdArgs->private_key_filename.c_str(), SSL_FILETYPE_PEM) <= 0) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(__func__, "Failed to load private key - {} file, err= {}", m_cmdArgs->private_key_filename, errMsg)
        exit(-1);
    }
    
    if (SSL_CTX_load_verify_locations(ctx, m_cmdArgs->ca_key_file_name.c_str(), nullptr) <= 0) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(__func__, "Failed to load verify locations - {} file, err= {}", m_cmdArgs->ca_key_file_name, errMsg)
        exit(-1);
    }
    
    if (!SSL_CTX_check_private_key(ctx)) {
        char errMsg[256];
        ERR_error_string_n(ERR_get_error(), errMsg, sizeof(errMsg));
        OATPP_LOGe(__func__, "Failed to SSL_CTX_check_private_key, err= {}", errMsg)
        exit(-1);
    }
    
    
    
    //SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
    SSL_CTX_set_verify_depth(ctx, 5); // certification chin limitation

    SSL_CTX_set_info_callback(ctx, printStateCB);
    
return ctx;
}

#endif //VGATEWAY_SSLCONTEXT_H
