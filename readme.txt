Basic program to transfer images/data over ethernet between two systems running Linux. Tested on Ubuntu 14.04 on the Enclustra Mercury ZX1 module on the custom board for the headlight project.

To install simply navigate to the root directory and run 'make'.

This code creates a listening(Server) and request thread(client) on respective devices that it is running.

Usage: 
Run the transfer program first on server device.
Then run the transfer program on client device.


SERVER] ./transfer <listening port that client will connect to>
CLIENT ] ./transfer <server IP address> <Port to connect to>


Client's port to connect to , should be same as server's listening port.


Example:
SERVER] ./transfer 6045
CLIENT] ./transfer 10.42.0.5 6045 7176
 
