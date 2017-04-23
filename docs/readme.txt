Basic program to transfer images/data over ethernet between two systems running Linux. Tested on Ubuntu 14.04 on the Enclustra Mercury ZX1 module on the custom board for the headlight project.

To install simply navigate to the root directory and run 'make'.


This code creates a listening(Server) and request thread(client) on both the devices that it is running.

Usage: CLIENT ] ./transfer <server address> <port to connect to> <listening port//doesn't matter for client//>

SERVER] ./transfer <host//doesn't matter for server//> <port to connect to//doesn't matter for server> <listening port that client will connect to>


Client's port to connect to , should be same as server's listening port.



SERVER] ./transfer 10.42.0.1 8123 6045
CLIENT] ./transfer 10.42.0.5 6045 7176
 
