/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2021, Live Networks, Inc.  All rights reserved
// A subclass of "RTSPServer" that creates "ServerMediaSession"s on demand,
// based on whether or not the specified stream name exists as a file
// Header file

#ifndef _CUSTOM_MEDIA_SUBSESSION_HH
#define _CUSTOM_MEDIA_SUBSESSION_HH

#include "OnDemandServerMediaSubsession.hh"

//创建一个自己的CustomMediaSubsession，继承OnDemandServerMediaSubsession；并实现createNewStreamSource和createNewRTPSink
class CustomMediaSubsession : public OnDemandServerMediaSubsession {
public:
    static OnDemandServerMediaSubsession *createNew(UsageEnvironment &env, char const *fileName,
                                                    Boolean reuseFirstSource = true);
protected:
    CustomMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource);
    //创建调用CustomFramedSource创建自定义的FramedSource
    virtual FramedSource *createNewStreamSource(unsigned clientSessionId, unsigned &estBitrate);
    //直接调用H264VideoRTPSink创建RTPSink
    virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
                                      unsigned char rtpPayloadTypeIfDynamic,
                                      FramedSource *inputSource);
protected:
    Groupsock *_rtpGroupsock;
};

#endif