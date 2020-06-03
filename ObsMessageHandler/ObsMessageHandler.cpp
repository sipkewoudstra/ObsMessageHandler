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

struct Position {
    int x = -5;
    int y = -5;
    int alignment = -5;
};

struct Scale {
    double x = -5;
    double y = -5;
};

struct Crop {
    int top = -5;
    int bottom = -5;
    int left = -5;
    int right = -5;
};

struct Bounds {
    std::string type = "NULL";
    int alignment = -5;
    int x = -5;
    int y = -5;
};

/* -------------------------------------------------------  Base64 encoding stuff --------------------------------------------------------------------------------------------- */

static const char b64_table[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char reverse_table[128] = {
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
   64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
   64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
   64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64
};

std::string base64_encode(const std::string &bindata)
{
   using std::string;
   using std::numeric_limits;

   if (bindata.size() > (numeric_limits<string::size_type>::max() / 4u) * 3u) {
      throw std::length_error("Converting too large a string to base64.");
   }

   const std::size_t binlen = bindata.size();
   // Use = signs so the end is properly padded.
   string retval((((binlen + 2) / 3) * 4), '=');
   std::size_t outpos = 0;
   int bits_collected = 0;
   unsigned int accumulator = 0;
   const string::const_iterator binend = bindata.end();

   for (string::const_iterator i = bindata.begin(); i != binend; ++i) {
      accumulator = (accumulator << 8) | (*i & 0xffu);
      bits_collected += 8;
      while (bits_collected >= 6) {
         bits_collected -= 6;
         retval[outpos++] = b64_table[(accumulator >> bits_collected) & 0x3fu];
      }
   }
   if (bits_collected > 0) { // Any trailing bits that are missing.
      assert(bits_collected < 6);
      accumulator <<= 6 - bits_collected;
      retval[outpos++] = b64_table[accumulator & 0x3fu];
   }
   assert(outpos >= (retval.size() - 2));
   assert(outpos <= retval.size());
   return retval;
}

std::string base64_decode(const std::string &ascdata)
{
   using std::string;
   string retval;
   const string::const_iterator last = ascdata.end();
   int bits_collected = 0;
   unsigned int accumulator = 0;

   for (string::const_iterator i = ascdata.begin(); i != last; ++i) {
      const int c = *i;
      if (std::isspace(c) || c == '=') {
         // Skip whitespace and padding. Be liberal in what you accept.
         continue;
      }
      if ((c > 127) || (c < 0) || (reverse_table[c] > 63)) {
         throw std::invalid_argument("This contains characters not legal in a base64 encoded string.");
      }
      accumulator = (accumulator << 6) | reverse_table[c];
      bits_collected += 6;
      if (bits_collected >= 8) {
         bits_collected -= 8;
         retval += static_cast<char>((accumulator >> bits_collected) & 0xffu);
      }
   }
   return retval;
}


/* -------------------------------------------------------------- hash 256 encoding stuff ------------------------------------------------------------------------------------------------   */

bool computeHash(const std::string& unhashed, std::string& hashed)
{
    bool success = false;

    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if(context != NULL)
    {
        if(EVP_DigestInit_ex(context, EVP_sha256(), NULL))
        {
            if(EVP_DigestUpdate(context, unhashed.c_str(), unhashed.length()))
            {
                unsigned char hash[EVP_MAX_MD_SIZE];
                unsigned int lengthOfHash = 0;

                if(EVP_DigestFinal_ex(context, hash, &lengthOfHash))
                {
                    std::stringstream ss;
                    for(unsigned int i = 0; i < lengthOfHash; ++i)
                    {
                        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
                    }

                    hashed = ss.str();
                    success = true;
                }
            }
        }

        EVP_MD_CTX_free(context);
    }

    return success;
}

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
    //untested
    
    std::string secret_string = _password + _salt, secret_hash;
    computeHash(secret_string, secret_hash);
    std::string secret = base64_encode(secret_hash);
    std::string auth_response_string = secret + _challenge, auth_response_hash;
    computeHash(auth_response_string, auth_response_hash);
    std::string auth_response = base64_encode(auth_response_hash);
    
    Json::Value root;
    
    root["message-id"] = std::to_string(AUTHENTICATE);
    root["request-type"] = "Authenticate";
    root["auth"] = auth_response;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
    
}

void ObsMessageHandler::r_SetHeartbeat(bool _enable)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETHEARTBEAT);
    root["request-type"] = "SetHeartbeat";
    root["enable"] = _enable;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_SetFilenameFormatting(std::string& _format)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETFILENAMEFORMATTING);
    root["request-type"] = "SetFilenameFormatting";
    root["filename-formatting"] = _format;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetFilenameFormatting()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETFILENAMEFORMATTING);
    root["request-type"] = "GetFilenameFormatting";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetStats()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETSTATS);
    root["request-type"] = "GetStats";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_BroadcastCustomMessage(std::string _realm, Json::Value& _object)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(BROADCASTCUSTOMMESSAGE);
    root["request-type"] = "BroadcastCustomMessage";
    root["realm"] = _realm;
    root["data"] = _object;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetVideoInfo()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETVIDEOINFO);
    root["request-type"] = "GetVideoInfo";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_OpenProjector(std::string _type = "NULL", int _monitor = -5, int _x = -5, int _y = -5, int _width = -5, int _height = -5, std::string _name = "NULL")
{
    //not tested
    Json::Value root;
    
    root["message-id"] = std::to_string(OPENPROJECTOR);
    root["request-type"] = "OpenProjector";
    
    std::string geometry = base64_encode(std::to_string(_x) + "," + std::to_string(_y) + "," + std::to_string(_width) + "," + std::to_string(_height));
    
    if(_type != "NULL") root["type"] = _type;
    if(_monitor != -5) root["monitor"] = _monitor;
    if(_x != -5 || _y != -5 || _width != -5 || _height != -5) root["geometry"] = geometry;
    if(_name != "NULL") root["name"] = _name;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_ListOutputs()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(LISTOUTPUTS);
    root["request-type"] = "ListOutputs";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetOutputInfo(std::string& _outputName)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETOUTPUTINFO);
    root["request-type"] = "GetOutputInfo";
    root["outputName"] = _outputName;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StartOutput(std::string& _outputName)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STARTOUTPUT);
    root["request-type"] = "StartOutput";
    root["outputName"] = _outputName;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StopOutput(std::string& _outputName, bool _force)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STOPOUTPUT);
    root["request-type"] = "StopOutput";
    root["outputName"] = _outputName;
    root["force"] = _force;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_SetCurrentProfile(std::string& _profileName)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETCURRENTPROFILE);
    root["request-type"] = "SetCurrentProfile";
    root["profile-name"] = _profileName;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetCurrentProfile()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETCURRENTPROFILE);
    root["request-type"] = "GetCurrentProfile";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_ListProfiles()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(LISTPROFILES);
    root["request-type"] = "ListProfiles";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StartStopRecording()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STARTSTOPRECORDING);
    root["request-type"] = "StartStopRecording";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StartRecording()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STARTRECORDING);
    root["request-type"] = "StartRecording";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StopRecording()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STOPRECORDING);
    root["request-type"] = "StopRecording";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_PauseRecording()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(PAUSERECORDING);
    root["request-type"] = "PauseRecording";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_ResumeRecording()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(RESUMERECORDING);
    root["request-type"] = "ResumeRecording";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_SetRecordingFolder(std::string& _recFolder)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETRECORDINGFOLDER);
    root["request-type"] = "SetRecordingFolder";
    root["rec-folder"] = _recFolder;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetRecordingFolder()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETRECORDINGFOLDER);
    root["request-type"] = "GetRecordingFolder";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StartStopReplayBufer()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STARTSTOPREPLAYBUFFER);
    root["request-type"] = "StartStopReplayBuffer";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StartReplayBuffer()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STARTREPLAYBUFFER);
    root["request-type"] = "StartReplayBuffer";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_StopReplayBuffer()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(STOPREPLAYBUFFER);
    root["request-type"] = "StopReplayBuffer";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_SaveReplayBuffer()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SAVEREPLAYBUFFER);
    root["request-type"] = "SaveReplayBuffer";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_SetCurrentSceneCollection(std::string& _scName)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETCURRENTSCENECOLLECTION);
    root["request-type"] = "SetCurrentSceneCollection";
    root["sc-name"] = _scName;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetCurrentSceneCollection()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETCURRENTSCENECOLLECTION);
    root["request-type"] = "GetCurrentSceneCollection";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_ListSceneCollections()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(LISTSCENECOLLECTIONS);
    root["request-type"] = "ListSceneCollections";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetSceneItemProperties(std::string _item, std::string _sceneName = "NULL", std::string _itemName = "NULL", int _itemId = -5)
{
    //add different function if item = object
    Json::Value root;
    
    root["message-id"] = std::to_string(GETSCENEITEMPROPERTIES);
    root["request-type"] = "GetSceneItemProperties";
    if(_sceneName != "NULL") root["scene-name"] = _sceneName;
    root["item"] = _item;
    if(_itemName != "NULL") root["item.name"] = _itemName;
    if(_itemId != -5) root["item.id"] = _itemId;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
    
}

void ObsMessageHandler::r_SetSceneItemProperties(std::string _item, std::string _sceneName = "NULL", std::string _itemName = "NULL", int _itemId = -5, Position _position = {-5, -5, -5}, double _rotation = -5, Scale _scale = {-5, -5}, Crop _crop = {-5, -5, -5, -5}, int _visible = -1, int _locked = -1, Bounds _bounds = {"NULL", -5, -5, -5})
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETSCENEITEMPROPERTIES);
    root["request-type"] = "SetSceneItemProperties";
    root["item"] = _item;
    if(_sceneName != "NULL")        root["scene-name"] = _sceneName;
    if(_itemName != "NULL")         root["item.name"] = _itemName;
    if(_itemId != -5)               root["item.id"] = _itemId;
    if(_position.x != -5)           root["position.x"] = _position.x;
    if(_position.y != -5)           root["position.y"] = _position.y;
    if(_position.alignment != -5)   root["position.alignment"] = _position.alignment;
    if(_rotation != -5)             root["rotation"] = _rotation;
    if(_scale.x != -5)              root["scale.x"] = _scale.x;
    if(_scale.y != -5)              root["scale.y"] = _scale.y;
    if(_crop.top != -5)             root["crop.top"] = _crop.top;
    if(_crop.bottom != -5)          root["crop.bottom"] = _crop.bottom;
    if(_crop.left != -5)            root["crop.left"] = _crop.left;
    if(_crop.right != -5)           root["crop.right"] = _crop.right;
    if(_visible != -1)              root["visible"] = _visible;
    if(_locked != -1)               root["locked"] = _locked;
    if(_bounds.type != "NULL")      root["bounds.type"] = _bounds.type;
    if(_bounds.alignment != -5)     root["bounds.alignment"] = _bounds.alignment;
    if(_bounds.x != -5)             root["bounds.x"] = _bounds.x;
    if(_bounds.y != -5)             root["bounds.y"] = _bounds.y;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_ResetSceneItem(std::string _item, std::string _sceneName = "NULL", std::string _itemName = "NULL", int _itemId = -5)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(RESETSCENEITEM);
    root["request-type"] = "ResetSceneItem";
    root["item"] = _item;
    if(_sceneName != "NULL")        root["scene-name"] = _sceneName;
    if(_itemName != "NULL")         root["item.name"] = _itemName;
    if(_itemId != -5)               root["item.id"] = _itemId;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_DeleteSceneItem(std::string _item, std::string _sceneName = "NULL", std::string _itemName = "NULL", int _itemId = -5)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(DELETESCENEITEM);
    root["request-type"] = "DeleteSceneItem";
    root["item"] = _item;
    if(_sceneName != "NULL")        root["scene-name"] = _sceneName;
    if(_itemName != "NULL")         root["item.name"] = _itemName;
    if(_itemId != -5)               root["item.id"] = _itemId;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_DuplicateSceneItem()
{
    //Not yet implimented
    std::cout << "NOT YET IMPLIMENTED" << std::endl;
}

void ObsMessageHandler::r_SetCurrentScene(std::string& _sceneName)
{
    Json::Value root;
    
    root["message-id"] = std::to_string(SETCURRENTSCENE);
    root["request-type"] = "SetCurrentScene";
    root["scene-name"] = _sceneName;
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetCurrentScene()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETCURRENTSCENE);
    root["request-type"] = "GetCurrentScene";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
}

void ObsMessageHandler::r_GetSceneList()
{
    Json::Value root;
    
    root["message-id"] = std::to_string(GETSCENELIST);
    root["request-type"] = "GetSceneList";
    
    Json::StreamWriterBuilder builder;
    const std::string json_file = Json::writeString(builder, root);
    
    ws.write(net::buffer(json_file));
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



/* -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------  */

void ObsMessageHandler::e_GetVersion(std::string somekindofingo)
{
    std::cout << "hoi" << std::endl;
}

int main()
{
    
    ObsMessageHandler messagehandler;
    std::string ip = "localhost";
    std::string port = "4444";
    messagehandler.connect(ip, port);
    std::string l = "Scene 2";
    messagehandler.r_GetSceneList();

    messagehandler.recieveUsingThread();
    std::cin.get();
    
    
    return 1;
}
