#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <err.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "net.h"
#include "jbod.h"

/* the client socket descriptor for the connection to the server */
int cli_sd = -1;

//JBOD_SERVER
//JBOD_PORT

/* attempts to read n (len) bytes from fd; returns true on success and false on failure. 
It may need to call the system call "read" multiple times to reach the given size len. 
*/
static bool nread(int fd, int len, uint8_t *buf) { //Probably done
  if (cli_sd == -1){ //Might need to check for more conditions
    return false;
  }
  int BytesRead = 0;
  while (len > 0){
    BytesRead = 0;
    BytesRead = read(fd, buf, len);
    
    if (BytesRead == -1){
      return false;
    }
    /*if (BytesRead == 0 && len != 0){
      return false;
    }*/
    if (BytesRead == 0){
      len -= len;
      buf += len;
    }
    else{
      buf += BytesRead;
      len -= BytesRead;
    }
  }
  return true;
}

/* attempts to write n bytes to fd; returns true on success and false on failure 
It may need to call the system call "write" multiple times to reach the size len.
*/
static bool nwrite(int fd, int len, uint8_t *buf) { //Probably done
  if (cli_sd == -1){ //Might need to check for more conditions
    return false;
  }
  int BytesWrote = 0;
  while (len > 0){
    BytesWrote = 0;
    BytesWrote = write(fd, buf, len);
    
    if (BytesWrote == -1){
      return false;
    }
    if (BytesWrote == 0){ //Is this right?
      len -= len;
      buf += len;
    }
    else{
      buf += BytesWrote;
      len -= BytesWrote;
    }
  }
  return true;
}

/* Through this function call the client attempts to receive a packet from sd 
(i.e., receiving a response from the server.). It happens after the client previously 
forwarded a jbod operation call via a request message to the server.  
It returns true on success and false on failure. 
The values of the parameters (including op, ret, block) will be returned to the caller of this function: 

op - the address to store the jbod "opcode"  
ret - the address to store the return value of the server side calling the corresponding jbod_operation function.
block - holds the received block content if existing (e.g., when the op command is JBOD_READ_BLOCK)

In your implementation, you can read the packet header first (i.e., read HEADER_LEN bytes first), 
and then use the length field in the header to determine whether it is needed to read 
a block of data from the server. You may use the above nread function here.  
*/
static bool recv_packet(int sd, uint32_t *op, uint16_t *ret, uint8_t *block) { //probably done
  if (cli_sd == -1){
    return false;
  }
  uint8_t header[HEADER_LEN]; //temp header array
  uint32_t temp_op; //temp op variable
  uint16_t temp_ret; //temp ret variable
  uint16_t temp_len; //temp len variable
  //int feedback = 0;
  
  if (nread(sd, HEADER_LEN, header) == false){
    return false;
  }

  
  //Do I need protocol for these values? YES
  
  memcpy(&temp_op, header + 2, 4); //op
  memcpy(&temp_ret, header + 6, 2); //ret
  memcpy(&temp_len, header, 2); //len
  
  uint16_t len = ntohs(temp_len); //len
  *op = ntohl(temp_op); //op
  *ret = ntohs(temp_ret);   //ret
  if (len > HEADER_LEN){
    if (nread(sd, JBOD_BLOCK_SIZE, block) == false){ 
      return false;
    }
  }
  return true;
}



/* The client attempts to send a jbod request packet to sd (i.e., the server socket here); 
returns true on success and false on failure. 

op - the opcode. 
block- when the command is JBOD_WRITE_BLOCK, the block will contain data to write to the server jbod system;
otherwise it is NULL.

The above information (when applicable) has to be wrapped into a jbod request packet (format specified in readme).
You may call the above nwrite function to do the actual sending.  
*/
static bool send_packet(int sd, uint32_t op, uint8_t *block) { //probably done
  if (cli_sd == -1){
    return false;
  }
  //Code goes here
  uint8_t packet[264];
  int len = 8; //8 if it isn't write block
  uint32_t temp_op = htonl(op); //op
  
  //Should anything here be based on protocol? YES   
  //Need to use memcpy, then use htons and htonl for len and op
  //ret is always 0 when sending
  memcpy(&packet[2], &temp_op, 4); //op
  packet[6] = 0; //ret values
  packet[7] = 0; //ret values
  //unint_32 command = op & 0x3F;
  if ((op & 0x3F)  == JBOD_WRITE_BLOCK){
    if (block == NULL){
      return false;
    }
    memcpy(&packet[8], block, 256);
    len = 264; //264 if it is write block
  }
  
  /*if (packet[1] == JBOD_WRITE_BLOCK && block != NULL){ //Extract command Make sure it is write block
    memcpy(&packet[4], block, 256);
    len = 264; //264 if it is write block
  }*/
  
  //Replaced with return nwrite();
  /*if (nwrite(sd, len, packet)){
    return true;
  }
  return false;*/
  uint16_t temp_len = htons(len); //len
  memcpy(&packet[0], &temp_len, 2); //len
  return nwrite(sd, len, packet);
}



/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. 
 * this function will be invoked by tester to connect to the server at given ip and port.
 * you will not call it in mdadm.c
*/
bool jbod_connect(const char *ip, uint16_t port) { //Definitely done
  //ip = JBOD_SERVER
  //port = JBOD_PORT
  cli_sd = socket(AF_INET, SOCK_STREAM, 0); //Might need to put this after test for valid address
  if (cli_sd == -1){
    return false;
  }
  //I will need connect()
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0){ //Tests for valid address
    return false;
  }
  if (connect(cli_sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){ //Connect used here to connect the socket that was made
    jbod_disconnect();
    return false;
  }
  return true;
}




/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) { //Definitely done
  if (cli_sd != -1){
    close(cli_sd);
    cli_sd = -1;
  }
}



/* sends the JBOD operation to the server (use the send_packet function) and receives 
(use the recv_packet function) and processes the response. 

The meaning of each parameter is the same as in the original jbod_operation function. 
return: 0 means success, -1 means failure.
*/
int jbod_client_operation(uint32_t op, uint8_t *block) { //done..?
  //The paramaters that ill need for these functions:
  //static bool send_packet(int sd, uint32_t op, uint8_t *block)
  //static bool recv_packet(int sd, uint32_t *op, uint16_t *ret, uint8_t *block)
  uint16_t ret;
  if(send_packet(cli_sd, op, block) == false){ 
    return -1;
  }
  else if (recv_packet(cli_sd, &op, &ret, block) == false){
    return -1;
  }

  if (ret == -1){
    return -1;
  }
  //Was told I didn't need this:
  //unint_32 command = op & 0x3F;
  /*if (((op >> 6) & 0x3F) == JBOD_READ_BLOCK || ((op >> 6) & 0x3F) == JBOD_SIGN_BLOCK){ //Issue here
    memcpy(block, &ret[1], 256);
    }*/
  return 0;
}
