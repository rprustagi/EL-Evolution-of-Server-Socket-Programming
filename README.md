# EL-Evolution-of-Server-Socket-Programming
Here we provide details on evolution of TCP server socket programming. The evolution would go thru following stages 

**Single client connection:**
Start from a server program that accepts requests from a single client and exits when client closes the connection 

**One client at a time**. 
The server program runs for ever, but handles one client at a time. When one client exits, only then it accepts requests from next clients and so on. 

**Limited number of Multiple concurrent clients:**
The server program accepts requests from a max number of concurrent clients as predefined when the server starts 

**Unlimited number of multiple concurrent clients:**
The server program accepts as many as clients as possible subject to availability of server resources.
