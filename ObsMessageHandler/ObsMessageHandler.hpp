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
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <iomanip>


namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;

using tcp = boost::asio::ip::tcp;


enum requestMessageId
{
    GETVERSION = 0,
    GETAUTHREQUIRED,
    AUTHENTICATE,
    SETHEARTBEAT,
    SETFILENAMEFORMATTING,
    GETFILENAMEFORMATTING,
    GETSTATS,
    BROADCASTCUSTOMMESSAGE,
    GETVIDEOINFO,
    OPENPROJECTOR,
    LISTOUTPUTS,
    GETOUTPUTINFO,
    STARTOUTPUT,
    STOPOUTPUT,
    SETCURRENTPROFILE,
    GETCURRENTPROFILE,
    LISTPROFILES,
    STARTSTOPRECORDING,
    STARTRECORDING,
    STOPRECORDING,
    PAUSERECORDING,
    RESUMERECORDING,
    SETRECORDINGFOLDER,
    GETRECORDINGFOLDER,
    STARTSTOPREPLAYBUFFER,
    STARTREPLAYBUFFER,
    STOPREPLAYBUFFER,
    SAVEREPLAYBUFFER,
    SETCURRENTSCENECOLLECTION,
    GETCURRENTSCENECOLLECTION,
    LISTSCENECOLLECTIONS,
    GETSCENEITEMPROPERTIES,
    SETSCENEITEMPROPERTIES,
    RESETSCENEITEM,
    DELETESCENEITEM,
    DUPLICATESCENEITEM,
    SETCURRENTSCENE,
    GETCURRENTSCENE,
    GETSCENELIST
    
};

struct Position;
struct Scale;
struct Crop;
struct Bounds;

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
    void r_SetHeartbeat(bool _enable);
    void r_SetFilenameFormatting(std::string& _format);
    void r_GetFilenameFormatting();
    void r_GetStats();
    void r_BroadcastCustomMessage(std::string _realm, Json::Value& _object);
    void r_GetVideoInfo();
    void r_OpenProjector(std::string _type, int _monitor, int _x, int _y, int _width, int _height, std::string _name);
    void r_ListOutputs();
    void r_GetOutputInfo(std::string& _outputName);
    void r_StartOutput(std::string& _outputName);
    void r_StopOutput(std::string& _outputName, bool _force);
    void r_SetCurrentProfile(std::string& _profileName);
    void r_GetCurrentProfile();
    void r_ListProfiles();
    void r_StartStopRecording();
    void r_StartRecording();
    void r_StopRecording();
    void r_PauseRecording();
    void r_ResumeRecording();
    void r_SetRecordingFolder(std::string& _recFolder);
    void r_GetRecordingFolder();
    void r_StartStopReplayBufer();
    void r_StartReplayBuffer();
    void r_StopReplayBuffer();
    void r_SaveReplayBuffer();
    void r_SetCurrentSceneCollection(std::string& _scName);
    void r_GetCurrentSceneCollection();
    void r_ListSceneCollections();
    void r_GetSceneItemProperties(std::string _item, std::string _sceneName, std::string _itemName, int _itemId);
    void r_SetSceneItemProperties(std::string _item, std::string _sceneName, std::string _itemName, int _itemId, Position _position, double _rotation, Scale _scale, Crop _crop, int _visible, int _locked, Bounds _bounds);
    void r_ResetSceneItem(std::string _item, std::string _sceneName, std::string _itemName, int _itemId);
    void r_DeleteSceneItem(std::string _item, std::string _sceneName, std::string _itemName, int _itemId);
    void r_DuplicateSceneItem(/* moet nog doen*/);
    void r_SetCurrentScene(std::string& _sceneName);
    void r_GetCurrentScene();
    void r_GetSceneList();
    
    void recieve();
    void recieveUsingThread();
    
private:
    friend void recieve();
    net::io_context ioc;
    tcp::resolver resolver{ioc};
    websocket::stream<tcp::socket> ws{ioc};
    

    
    std::thread recieveThread;
    std::mutex recieveMutex;

};

#pragma GCC visibility pop
#endif

