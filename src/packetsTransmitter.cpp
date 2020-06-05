#include "packetsTransmitter.h"
#include <iostream>

PacketsTransmitter::PacketsTransmitter(std::string ipAddr, std::string p):
addr(ipAddr), port(p), type(connectIP)
{

}

PacketsTransmitter::PacketsTransmitter(std::string btAddr, uint8_t ch):
addr(btAddr), channel(ch), type(connectBT)
{

}

PacketsTransmitter::PacketsTransmitter(std::string p):
port(p), type(listenIP)
{

}

PacketsTransmitter::PacketsTransmitter(uint8_t ch):
channel(ch), type(listenBT)
{

}