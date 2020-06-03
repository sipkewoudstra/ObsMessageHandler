//
//  ObsMessageHandler.hpp
//  ObsMessageHandler
//
//  Created by sipke woudstra on 01/06/2020.
//  Copyright © 2020 sipke woudstra. All rights reserved.
//

#ifndef ObsMessageHandler_
#define ObsMessageHandler_

/* The classes below are exported */
#pragma GCC visibility push(default)

#include <thread>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <json.h>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = boost::asio::ip::tcp;

enum requestMessageId
{
    GETVERSION = 0,
    GETAUTHREQUIRED,
    AUTHENTICATE
    
};

class ObsMessageHandler
{
public:
    
    ObsMessageHandler();
    ~ObsMessageHandler();
    
    bool connect(std::string& _ip, std::string& _port);
    
    //requests see:https://github.com/Palakis/obs-websocket/blob/4.x-current/docs/generated/protocol.md
    
    void r_GetVersion();
    void r_GetAuthRequired();
    void r_Authenticate(std::string& _challenge, std::string& _salt, std::string& _password);
    
    
    
    
    
    void recieve();
    void recieveUsingThread();
    
    
private:
    friend void recieve();
    //std::thread messageHandlerThread;
    net::io_context ioc;
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};

    
    std::thread recieveThread;
    std::mutex recieveMutex;

};

#pragma GCC visibility pop
#endif
