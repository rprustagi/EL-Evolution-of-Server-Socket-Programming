#!/usr/bin/python
import socket
import time
import argparse
import os

parser = argparse.ArgumentParser(description="Simple Server for N/w Delays")
parser.add_argument('-s', '--server', type=str, default="0.0.0.0")
parser.add_argument('-p', '--port', type=int, default=9999)
parser.add_argument('-b', '--buffer',  type=int, default=1000)
parser.add_argument('-c', '--cnt_children',  type=int, default=10)
args = parser.parse_args()

ip_addr = args.server
port = args.port
buffer = args.buffer
cnt_children = args.cnt_children


# ----------------------------------------------
def child():
  while True:
    connsock, client = sock.accept()
    print("client", client, "is processed by child", os.getpid())
    while True:
      data = connsock.recv(buffer)
      if not data:
        break
      connsock.send(data.decode().upper().encode())
    connsock.close()
# ----------------------------------------------
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
srvr_addr = (ip_addr, port)
sock.bind(srvr_addr)
sock.listen(1)
for ii in range(cnt_children): # create predefined number of child processes
  childpid = os.fork()
  if childpid == 0:
    child()

while True: # perpetual loop to accept next client request
    time.sleep(1)
# code should never reach here.
