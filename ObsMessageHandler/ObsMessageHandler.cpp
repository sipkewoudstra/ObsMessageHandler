//
//  ObsMessageHandler.cpp
//  ObsMessageHandler
//
//  Created by sipke woudstra on 01/06/2020.
//  Copyright © 2020 sipke woudstra. All rights reserved.
//

#include <iostream>
#include "ObsMessageHandler.hpp"
#include "ObsMessageHandlerPriv.hpp"

ObsMessageHandler::ObsMessageHandler(){
}

ObsMessageHandler::~ObsMessageHandler(){
    ws.close(websocket::close_code::normal);
    recieveThread.detach();
}

bool ObsMessageHandler::connect(std::string& _host, std::string& _port)
{
    try
    {
        auto const results = resolver.resolve(_host, _port);
        net::connect(ws.next_layer(), results.begin(), results.end());
        
        ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));
        
        ws.handshake(_host, "/");
        
        /* test code  */
        
        /*Json::Value root;
        
        root["message-id"] = "1";
        root["request-type"] = "GetVersion";
        
        Json::StreamWriterBuilder builder;
        const std::string json_file = Json::writeString(builder, root);
        
        ws.write(net::buffer(json_file));
         */
        
    }
    catch(std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return true;
}

void ObsMessageHandler::r_GetVersion()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETVERSION);
    root["request-type"] = "GetVersion";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetAuthRequired()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETAUTHREQUIRED);
    root["request-type"] = "GetAuthRequired";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_Authenticate(std::string& _challenge, std::string& _salt, std::string& _password)
{
    std::string secret_string = _password + _salt;
    std::string secret_hash = 
}


void ObsMessageHandler::recieve()
{
    
    while(1){
        try
        {
            beast::flat_buffer buffer;

            // Read a message into our buffer
            std::lock_guard<std::mutex> lock(recieveMutex);
            ws.read(buffer);

            // The make_printable() function helps print a ConstBufferSequence
            std::cout << beast::make_printable(buffer.data()) << std::endl;
        }
        catch(std::exception const& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
}

void ObsMessageHandler::recieveUsingThread(){
    recieveThread = std::thread(&ObsMessageHandler::recieve, this);
}


/* -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------  */

int main()
{
    
    ObsMessageHandler messagehandler;
    std::string ip = "localhost";
    std::string port = "4444";
    messagehandler.connect(ip, port);
    //messagehandler.r_GetVersion();
    //messagehandler.r_GetAuthRequired();
    messagehandler.recieveUsingThread();
    
    
    
    
    
    std::cin.get();
    
    
    return 1;
}
