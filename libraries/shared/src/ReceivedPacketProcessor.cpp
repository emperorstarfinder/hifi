//
//  ReceivedPacketProcessor.cpp
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet receiver.
//

#include "ReceivedPacketProcessor.h"

void ReceivedPacketProcessor::queuePacket(sockaddr& address, unsigned char* packetData, ssize_t packetLength) {
    NetworkPacket packet(address, packetData, packetLength);
    lock();
    _packets.push_back(packet);
    unlock();
}

bool ReceivedPacketProcessor::process() {
    if (_packets.size() == 0) {
        const uint64_t RECEIVED_THREAD_SLEEP_INTERVAL = (1000 * 1000)/60; // check at 60fps
        usleep(RECEIVED_THREAD_SLEEP_INTERVAL);
    }
    while (_packets.size() > 0) {
        NetworkPacket& packet = _packets.front();
        processPacket(packet.getAddress(), packet.getData(), packet.getLength());

        lock();
        _packets.erase(_packets.begin());
        unlock();
    }
    return true;  // keep running till they terminate us
}
