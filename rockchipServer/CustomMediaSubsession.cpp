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
// "liveMedia"
// Copyright (c) 1996-2021 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 video file.
// Implementation

#include "CustomMediaSubsession.hh"

#include "FramedSource.hh"
#include "H264VideoRTPSink.hh"
#include "H264VideoRTPSource.hh"

static int createSocket(void) {
    int port = 5600;
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        printf("Create udp socket failed\n");
        return -1;
    }

    // int on = 1;
    // setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    // return socket_fd;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = 0;

    if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        printf("bind socket port :%d failed \n", port);
        return -1;
    }
    return socket_fd;
}

//首先继承class FramedSource，定义一个自己的 CustomFramedSource，从socket中获取实时码流
class CustomFramedSource : public FramedSource {
public:
    static CustomFramedSource *createNew(UsageEnvironment &env, unsigned preferredFrameSize = 0,
                                         unsigned playTimePerFrame = 0) {
        //在创建CustomFramedSource的时候创建一个connectServer链接
        int socket = createSocket();
        if (socket < 0) return NULL;
        CustomFramedSource *newSource =
            new CustomFramedSource(env, socket, preferredFrameSize, playTimePerFrame);
        newSource->fFileSize = 0xFFFFFFFF;
        return newSource;
    }
    //CustomFramedSource 被销毁时记得关闭socket，不然会出现socket泄露
    //CustomFramedSource将会在客户端终止播放的时候被销毁
    ~CustomFramedSource() {
        printf("CustomFramedSource close socket=%d\n", m_socket);
        if (m_socket > 0) {
            ::close(m_socket);
        }
        m_socket = -1;
    }

    u_int64_t fileSize() const { return fFileSize; }
protected:
    CustomFramedSource(UsageEnvironment &env, int socket, unsigned preferredFrameSize,
                       unsigned playTimePerFrame)
        : FramedSource(env),
          m_preferredFrameSize(preferredFrameSize),
          m_playTimePerFrame(playTimePerFrame),
          m_socket(socket) {
        envir() << "Create New CustomFramedSource\n";
        rtpBufferSize = 1600;
        rtpBuffer = (unsigned char *)malloc(rtpBufferSize);
    }
    //我们只需要实现doGetNextFrame函数，将实时码流读到fTo中，将大小填到fFrameSize中
    virtual void doGetNextFrame() {
        printf("call do get next frame\n");
        fFrameSize = recv(m_socket, rtpBuffer, rtpBufferSize, 0);
        fFrameSize -= 12;
        memcpy(fTo, rtpBuffer + 12, fFrameSize);
        printf("frame size : %d\n", fFrameSize);
        if (fFrameSize <= 0) {
            handleClosure();
            ::close(m_socket);
            return;
        }

        gettimeofday(&fPresentationTime, NULL);
        // Because the file read was done from the event loop, we can call the
        // 'after getting' function directly, without risk of infinite recursion:
        FramedSource::afterGetting(this);
    }
protected:
    unsigned m_preferredFrameSize;
    unsigned m_playTimePerFrame;
    unsigned fLastPlayTime;
    unsigned fFileSize;
    int m_socket = -1;
    unsigned char *rtpBuffer;
    int rtpBufferSize;
};

OnDemandServerMediaSubsession *CustomMediaSubsession::createNew(
    UsageEnvironment &env, char const *fileName, Boolean reuseFirstSource /*= true*/) {
    //return H264VideoFileServerMediaSubsession::createNew(env, "goertek.264", reuseFirstSource);
    return new CustomMediaSubsession(env, reuseFirstSource);
}

CustomMediaSubsession::CustomMediaSubsession(UsageEnvironment &env, Boolean reuseFirstSource)
    : OnDemandServerMediaSubsession(env, reuseFirstSource) {
    struct sockaddr_storage tempAddr = nullAddress();
    _RTPgs = new Groupsock(envir(), tempAddr, nullAddress(), 5600);
}

//创建调用CustomFramedSource创建自定义的FramedSource
FramedSource *CustomMediaSubsession::createNewStreamSource(unsigned clientSessionId,
                                                           unsigned &estBitrate) {
    estBitrate = 10000;  // kbps, estimate
    // Create the video source:
    // CustomFramedSource* fileSource = CustomFramedSource::createNew(envir());
    // if (fileSource == NULL) return NULL;

    // Create a framer for the Video Elementary Stream:
    return H264VideoRTPSource::createNew(envir(), _RTPgs, 96);
    //return H264VideoStreamFramer::createNew(envir(), fileSource);
}

//直接调用H264VideoRTPSink创建RTPSink
RTPSink *CustomMediaSubsession::createNewRTPSink(Groupsock *rtpGroupsock,
                                                 unsigned char rtpPayloadTypeIfDynamic,
                                                 FramedSource *inputSource) {
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}