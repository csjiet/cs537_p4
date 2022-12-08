#include <stdio.h>
#include "udp.h"

#include "message.h"

int run_init(message_t* m){
	return -1;
}

int run_lookup(message_t* m){

	// Search in superblock


    // Search in data bitmap

    // Look into data region pointed by the data bitmap


	// This is a test. It is not done properly here!
	m->c_received_data = (char*)malloc(sizeof(char) * 28);
	strcpy(m->c_received_data, "Wow data here!\n");

	return 0;
}

int run_stat(message_t* m){
	return -1;
}

int run_write(message_t* m){
	return -1;
}

int run_read(message_t* m){
	return -1;
}

int run_cret(message_t* m){
	return -1;
}

int run_unlink(message_t* m){

	return -1;
}

int run_shutdown(message_t* m){
	return -1;
}


// This function checks the c_sent_mtype field from message_t struct - which identifies the client function
// (e.g., MFS_WRITE()) who submitted the request - and calls the respective function to propogate the message_t 
// struct with the proper data.
int message_parser(message_t* m){

	// Decode message_t 
	int message_func = m->c_sent_mtype;
	int rc;

	if(message_func == MFS_INIT){
		rc = run_init(m);

	}else if(message_func == MFS_LOOKUP){
		rc = run_lookup(m);

	}else if(message_func == MFS_STAT){
		rc = run_stat(m);

	}else if(message_func == MFS_WRITE){
		rc = run_write(m);

	}else if(message_func == MFS_READ){
		rc = run_read(m);

	}else if(message_func == MFS_CRET){
		rc = run_cret(m);

	}else if(message_func == MFS_UNLINK){
		rc = run_unlink(m);

	}else if(message_func == MFS_SHUTDOWN){
		rc = run_shutdown(m);

	}

	return rc;
}

// server code

// Edited server main function:
int main(int argc, char *argv[]) {
    int sd = UDP_Open(10000); // Opens server socket with port 10000
    assert(sd > -1);
    while (1) {
	struct sockaddr_in addr;

	// Create a message_t struct to store data received from client
	message_t m;
	
	// Read data from client. rc >0 denotes a successful receive from a client.
	int rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
	if (rc > 0) {
		rc = message_parser(&m);

		// Assigns message_t return code to see if server side operation is successful
		m.c_received_rc = rc;

		rc = UDP_Write(sd, &addr, (char *) &m, sizeof(message_t));
	} 
    }
    return 0; 
}

// Original server main function 
// int main(int argc, char *argv[]) {
//     int sd = UDP_Open(10000); // Opens server socket with port 10000
//     assert(sd > -1);
//     while (1) {
// 	struct sockaddr_in addr;

// 	message_t m;
	
// 	printf("server:: waiting...\n");
// 	int rc = UDP_Read(sd, &addr, (char *) &m, sizeof(message_t));
// 	printf("server:: read message [size:%d contents:(%d)]\n", rc, m.mtype);
// 	if (rc > 0) {
// 	    m.rc = 3;
//             rc = UDP_Write(sd, &addr, (char *) &m, sizeof(message_t));
// 	    printf("server:: reply\n");
// 	} 
//     }
//     return 0; 
// }
    


 
