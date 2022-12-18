# cs537_p4

Authors: Collin Lull and Sui Jiet Tay

Title: Distributed File System

##########################################################################
# STEPS TO RUN PROGRAM IF TEST IS NOT WORKING:
# 1. make
# 2. export LD_LIBRARY_PATH=.
# 1. make
# 2. Check for library existing
# 3. run tests with shell script.
##########################################################################

Thought process:
1. Implement the API calls for client at client.c
2. Make sure the client and server socket connection is made with proper sending of structs
3. Design the struct to hold all data types needed for client and server communication
4. Implement the server functions in server.c
5. Write an initial implementation starting from the most basic - creat() function. 
6. Run test and reiterate debugging process.


