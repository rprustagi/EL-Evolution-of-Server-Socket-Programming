#!/usr/bin/python
# program using select for reading from socket
import socket
import select
import time
import argparse
import os

parser = argparse.ArgumentParser(description="Simple Server for N/w Delays")
parser.add_argument('-s', '--server', type=str, default="0.0.0.0")
parser.add_argument('-p', '--port', type=int, default=9999)
parser.add_argument('-b', '--buffer',  type=int, default=1000)
parser.add_argument('-t', '--timeout',  type=int, default=10)
args = parser.parse_args()

ip_addr = args.server
port = args.port
buffer = args.buffer
timeout = args.timeout

# create a listening socket that will wait for new connections
lsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
lsock.bind((ip_addr, port))
lsock.listen(1)

# create a list of active client sockets
csocks = []

while True:
    # get active sockets (reading, and error(closed))
    (rlist, wlist, elist) = select.select([lsock] + csocks, [], csocks, timeout)

    if not (rlist or wlist or elist):
        print ("select() call timed out for ", timeout, " seconds")
    # process client requests that have sent data
    for rsock in rlist:
        if rsock is lsock: # a new connection has arrived
            newsock, client = lsock.accept()
            csocks.append(newsock)
            print ("Received new connection from ", client)
        else: # data is available to read on existing connection
            message = rsock.recv(buffer)
            if not message:
                rsock.close()
                csocks.remove(rsock)
            else:
                rsock.send(message.decode().upper().encode())
    # close all the sockets on which error has occurred
    for errsock in elist:
        errsock.close()
# code should never reach here.

