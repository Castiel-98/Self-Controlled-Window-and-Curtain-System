#!/usr/bin/env python
# -*- coding: utf-8 -*-

from threading import Thread
from zeroconf import ServiceInfo, Zeroconf
from SimpleWebSocketServer import SimpleWebSocketServer, WebSocket
from Blinker.BlinkerConfig import *
from Blinker.BlinkerDebug import *
from BlinkerUtility.BlinkerUtility import *

clients = []
deviceName = macDeviceName()
deviceIP = localIP()

class WS_Protol():
    msgBuf = ''
    isRead = False
    state = CONNECTING
    debug = BLINKER_DEBUG

wsProto = WS_Protol()

# def isDebugAll():
#     if wsProto.debug == BLINKER_DEBUG_ALL:
#         return True
#     else:
#         return False

def mDNSinit(type, name):
    deviceType = '_' + type
    desc = {'deviceName': name}
    # desc = {}

    info = ServiceInfo(deviceType + "._tcp.local.",
                       name + "." + deviceType +"._tcp.local.",
                       socket.inet_aton(deviceIP), wsPort, 0, 0,
                       desc, name + ".local.")

    zeroconf = Zeroconf()
    zeroconf.register_service(info)

    # if isDebugAll() is True:
    BLINKER_LOG_ALL('deviceIP: ', deviceIP)
    BLINKER_LOG_ALL('mdns name: ', name)

    BLINKER_LOG('mDNS responder init!')

class HandleServer(WebSocket):

    def handleMessage(self):
        wsProto.msgBuf = self.data
        wsProto.isRead = True
        # if isDebugAll() is True:
        BLINKER_LOG_ALL('Read data: ', self.data)
        
    def handleConnected(self):
        clients.append(self)
        msg = json_encode(BLINKER_CMD_STATE, BLINKER_CMD_CONNECTED)
        for client in clients:
            client.sendMessage(msg)
        BLINKER_LOG(self.address, 'connected')
        wsProto.state = CONNECTED
        # freshState(CONNECTED)
        # bProto.state = CONNECTED

    def handleClose(self):
        clients.remove(self)
        BLINKER_LOG(self.address, 'closed')
        if len(clients) == 0:
            wsProto.state = DISCONNECTED
        #     bProto.state = DISCONNECTED
        #     freshState(DISCONNECTED)

class WebSocketServer(Thread):
    # def __init__(self, name, port, type = BLINKER_DIY_WIFI):
    def __init__(self, type = BLINKER_DIY_WIFI):
        Thread.__init__(self)
        self.name = deviceIP
        self.port = wsPort
        self.type = type
        self.server = SimpleWebSocketServer(self.name, self.port, HandleServer)
        self._isClosed = False
        self.setDaemon(True)

    def start(self, name = deviceName):
        mDNSinit(self.type , name)
        BLINKER_LOG('websocket Server init')
        BLINKER_LOG('ws://', self.name, ':', self.port)
        super(WebSocketServer, self).start()

    def run(self):
        self.server.serveforever()

    def stop(self):
        self.server.close()
        self._isClosed = True

    def broadcast(self, msg):
        msg = json.dumps(msg)
        # if isDebugAll() is True:
        BLINKER_LOG_ALL('Response data: ', msg)
        if len(clients) == 0:
            wsProto.state = DISCONNECTED
            # if isDebugAll() is True:
            BLINKER_ERR_LOG_ALL('Faile... Disconnected')
            return
        # if isDebugAll() is True:
        BLINKER_LOG_ALL('Succese...')
        if isinstance(msg, str):
            msg = msg.encode('utf-8').decode("utf-8")
        for client in clients:
            client.sendMessage(msg)
            while client.sendq:
                opcode, payload = client.sendq.popleft()
                remaining = client._sendBuffer(payload)
                if remaining is not None:
                    client.sendq.appendleft((opcode, remaining))
                    break

# bWSServer = WebSocketServer(deviceIP, wsPort)
