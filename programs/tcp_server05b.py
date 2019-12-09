#!/usr/bin/python
import socket
import time
import argparse
import os

parser = argparse.ArgumentParser(description="Simple Server for N/w Delays")
parser.add_argument('-s', '--server', type=str, default="0.0.0.0")
parser.add_argument('-p', '--port', type=int, default=9999)
parser.add_argument('-b', '--buffer',  type=int, default=1000)
args = parser.parse_args()

ip_addr = args.server
port = args.port
buffer = args.buffer

# ----------------------------------------------
def child():
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
while True: # perpetual loop to accept next client request
  connsock, client = sock.accept()
  childpid = os.fork()
  if childpid == 0:
    child()
  else:
    print("Created a child with pid", childpid, "for child connection", client)
  os.waitpid(-1, os.WNOHANG)
# code should never reach here.
