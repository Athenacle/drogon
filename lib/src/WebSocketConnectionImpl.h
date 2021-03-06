/**
 *
 *  WebSocketConnectionImpl.h
 *  An Tao
 *
 *  Copyright 2018, An Tao.  All rights reserved.
 *  https://github.com/an-tao/drogon
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Drogon
 *
 */

#pragma once

#include "impl_forwards.h"
#include <drogon/WebSocketConnection.h>
#include <trantor/utils/NonCopyable.h>
#include <trantor/net/TcpConnection.h>

#include "HttpAppFrameworkImpl.h"
namespace drogon
{
class WebSocketConnectionImpl;
using WebSocketConnectionImplPtr = std::shared_ptr<WebSocketConnectionImpl>;

class WebSocketMessageParser
{
    HttpAppFrameworkImpl *app_;

  public:
    WebSocketMessageParser(HttpAppFrameworkImpl *app) : app_(app)
    {
    }

    bool parse(trantor::MsgBuffer *buffer);
    bool gotAll(std::string &message, WebSocketMessageType &type)
    {
        assert(message.empty());
        if (!gotAll_)
            return false;
        message.swap(message_);
        type = type_;
        return true;
    }

  private:
    std::string message_;
    WebSocketMessageType type_;
    bool gotAll_{false};
};

class WebSocketConnectionImpl
    : public WebSocketConnection,
      public std::enable_shared_from_this<WebSocketConnectionImpl>,
      public trantor::NonCopyable
{
  public:
    explicit WebSocketConnectionImpl(const trantor::TcpConnectionPtr &conn,
                                     HttpAppFrameworkImpl *app,
                                     bool isServer = true);

    virtual void send(
        const char *msg,
        uint64_t len,
        const WebSocketMessageType type = WebSocketMessageType::Text) override;
    virtual void send(
        const std::string &msg,
        const WebSocketMessageType type = WebSocketMessageType::Text) override;

    virtual const trantor::InetAddress &localAddr() const override;
    virtual const trantor::InetAddress &peerAddr() const override;

    virtual bool connected() const override;
    virtual bool disconnected() const override;

    virtual void shutdown(
        const CloseCode code = CloseCode::kNormalClosure,
        const std::string &reason = "") override;  // close write
    virtual void forceClose() override;            // close

    virtual void setPingMessage(
        const std::string &message,
        const std::chrono::duration<long double> &interval) override;

    void setMessageCallback(
        const std::function<void(std::string &&,
                                 const WebSocketConnectionImplPtr &,
                                 const WebSocketMessageType &)> &callback)
    {
        messageCallback_ = callback;
    }

    void setCloseCallback(
        const std::function<void(const WebSocketConnectionImplPtr &)> &callback)
    {
        closeCallback_ = callback;
    }

    void onNewMessage(const trantor::TcpConnectionPtr &connPtr,
                      trantor::MsgBuffer *buffer);

    void onClose()
    {
        if (pingTimerId_ != trantor::InvalidTimerId)
            tcpConnectionPtr_->getLoop()->invalidateTimer(pingTimerId_);
        closeCallback_(shared_from_this());
    }

  private:
    trantor::TcpConnectionPtr tcpConnectionPtr_;
    trantor::InetAddress localAddr_;
    trantor::InetAddress peerAddr_;
    bool isServer_{true};
    WebSocketMessageParser parser_;
    trantor::TimerId pingTimerId_{trantor::InvalidTimerId};

    std::function<void(std::string &&,
                       const WebSocketConnectionImplPtr &,
                       const WebSocketMessageType &)>
        messageCallback_ = [](std::string &&,
                              const WebSocketConnectionImplPtr &,
                              const WebSocketMessageType &) {};
    std::function<void(const WebSocketConnectionImplPtr &)> closeCallback_ =
        [](const WebSocketConnectionImplPtr &) {};
    void sendWsData(const char *msg, uint64_t len, unsigned char opcode);
};

}  // namespace drogon
