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

void ObsMessageHandler::HelloWorld(const char * s)
{
    ObsMessageHandlerPriv *theObj = new ObsMessageHandlerPriv;
    theObj->HelloWorldPriv(s);
    delete theObj;
};

void ObsMessageHandlerPriv::HelloWorldPriv(const char * s) 
{
    std::cout << s << std::endl;
};

