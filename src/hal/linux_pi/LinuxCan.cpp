#include "LinuxCan.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>

LinuxCan::LinuxCan(const std::string& ifName) : _ifName(ifName) {}

LinuxCan::~LinuxCan() {
    stop();
}

bool LinuxCan::begin(uint32_t baudRate, int8_t rxPin, int8_t txPin) {
    (void)baudRate;
    (void)rxPin;
    (void)txPin;

    stop();

    _sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (_sock < 0) {
        _connected = false;
        return false;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, _ifName.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(_sock, SIOCGIFINDEX, &ifr) < 0) {
        close(_sock);
        _sock = -1;
        _connected = false;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(_sock);
        _sock = -1;
        _connected = false;
        return false;
    }

    _connected = true;
    return true;
}

bool LinuxCan::sendFrame(const CanFrame& frame) {
    if (_sock < 0) return false;

    struct can_frame cf;
    std::memset(&cf, 0, sizeof(cf));
    cf.can_id = frame.id;
    if (frame.extended) {
        cf.can_id |= CAN_EFF_FLAG;
    }
    cf.can_dlc = frame.len > 8 ? 8 : frame.len;
    std::memcpy(cf.data, frame.data, cf.can_dlc);

    ssize_t nbytes = write(_sock, &cf, sizeof(cf));
    return (nbytes == sizeof(cf));
}

bool LinuxCan::receiveFrame(CanFrame& frame, uint32_t timeoutMs) {
    if (_sock < 0) return false;

    struct pollfd pfd;
    pfd.fd = _sock;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, (int)timeoutMs);
    if (ret <= 0) return false;

    struct can_frame cf;
    ssize_t nbytes = read(_sock, &cf, sizeof(cf));
    if (nbytes != sizeof(cf)) return false;

    frame.extended = (cf.can_id & CAN_EFF_FLAG) != 0;
    frame.id = cf.can_id & (frame.extended ? CAN_EFF_MASK : CAN_SFF_MASK);
    frame.len = cf.can_dlc;
    std::memcpy(frame.data, cf.data, frame.len);

    return true;
}

bool LinuxCan::isConnected() const {
    return _connected && (_sock >= 0);
}

void LinuxCan::stop() {
    if (_sock >= 0) {
        close(_sock);
        _sock = -1;
    }
    _connected = false;
}
