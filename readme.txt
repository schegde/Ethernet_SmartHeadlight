Single threaded Server-client model based data communication over ethernet interface between 2 linux systems, 
specifically optimized for the embedded linux environment of Smart Headlight project at ILIM,CMU RI.


Tested on Ubuntu 14.04 x86 image on PC, and ARM-HF image compiled for Enclustra Mercury ZX1 module on the custom board 
of headlight project.

To install simply navigate to the root directory and run 'make'.


As per project requirements, the server role would be suited for the device, which will have all 
the allocated buffers, while client role would be for the device which will request and receive these buffers.

Usage: 
1) After hitting make, just run the program : ./transfer
2) This code begins with an option to chose the mode of operation -Server mode or client mode. 
3) Run this program first on server device(in server role), and then run the program on client device(in client role.). 
   This program running order is necessary!

NOTE: Master branch hosts a demo application(transfer.c) that shows all the functionality required. The external_driver branch shows an 	  external C application(transfer_app.c) controlling the transfer program through the transfer.h API.
 
