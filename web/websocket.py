#Modified to read IQ data from FIFO, compute FFT and send to client

import datetime
import json
import numpy as np
from numpy.fft import fft, fftfreq
from collections import deque
from threading import Thread
import struct
import signal
import random
import time

import tornado.httpserver
import tornado.ioloop
import tornado.web
from tornado import websocket

#awful globals :/
global fft_frame
fft_frame = np.arange(0,512,1.)
global killthread
killthread = False

def signal_handler( signal, frame):
    print "Killing DSP()"
    killthread = True

def DSP( fifoname, fftlen):
    global fft_frame
    global killthread
    fft_buff = deque( np.zeros( fftlen, dtype='complex'), fftlen)
    with open(fifoname,'r') as fifo:
        print "Opened fifo"
        read_buff = ""
        window_cnt = 1
        while True:
            read_buff += fifo.read(1)
            if len(read_buff) == 4:
                data = struct.unpack( "HH", read_buff )
                read_buff = ""
                fft_buff.pop()
                fft_buff.appendleft( complex(data[0], data[1]))
                window_cnt = window_cnt + 1
            #For now let's do an fft every fftlen ( zero windowing)
            if( window_cnt % (fftlen/2) == 0):
                print "NEW FFT READY"
                fft_frame = np.absolute(fft( fft_buff ))
                fft_frame = np.fft.fftshift(fft_frame)
                window_cnt = 0
            # print "Read data failed"
            # pass

            if killthread:
                break


def test_fft():
  freqs = np.arange(0,512,1.)
  random.shuffle( freqs )
  freq_list = freqs.tolist()
  return freq_list

class EchoWebSocket( websocket.WebSocketHandler ):
    def open( self ):
        print "WebSocket opened"
        self.closed = False
        self.data()

    def on_message( self, message ):
        #print 'Received message: %s' % message
        self.data()

    def close( self ):
        self.closed = True
        time.sleep( 1.1 )
        websocket.WebSocketHandler.close( self )

    def on_close( self ):
        print "WebSocket closed"

    def data( self ):
        #absfreq = test_fft()
        #message = json.dumps(absfreq)
        global fft_frame
        message = json.dumps( fft_frame.tolist() )

        self.write_message(message)

def main( ):
    signal.signal( signal.SIGINT, signal_handler )
    thread = Thread( target = DSP, args = ( "/tmp/myfifo", 512 ))
    thread.start()
    application = tornado.web.Application([
        (r'/ws', EchoWebSocket),
    ])

    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(8888)
    tornado.ioloop.IOLoop.instance().start()

if __name__ == '__main__':
    main()

