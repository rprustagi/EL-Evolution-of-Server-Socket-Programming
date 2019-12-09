#!/usr/bin/python
import socket
import time
import argparse

parser = argparse.ArgumentParser(description="Simple Server for N/w Delays")
parser.add_argument('-s', '--server', type=str, default="0.0.0.0")
parser.add_argument('-p', '--port', type=int, default=9999)
parser.add_argument('-b', '--buffer',  type=int, default=1000)
args = parser.parse_args()

ip_addr = args.server
port = args.port
buffer = args.buffer

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srvr_addr = (ip_addr, port)
sock.bind(srvr_addr); #print(time.asctime())
#time.sleep(60)
sock.listen(1); print(time.asctime())
time.sleep(60); print(time.asctime())
connsock, client = sock.accept()

while True:
    data = connsock.recv(buffer)
    if not data:
        break
    print(data.decode())
connsock.close()

