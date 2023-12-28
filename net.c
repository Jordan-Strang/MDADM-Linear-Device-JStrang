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

/* attempts to read n bytes from fd; returns true on success and false on
 * failure */
static bool nread(int fd, int len, uint8_t *buf) {

  //A tracker that keeps track of the number of bytes that have been read
  int bytes_read = 0;

  //While the length is greater than the bytes read, run the loop
  while(len > bytes_read){
    
    //Reads the bytes
    int read_bytes = read(fd, &buf[bytes_read], (len - bytes_read));

    //If the read bytes is less than or equal to zero, return that the process was a failure
    if(read_bytes <= 0)
      return false;

    //The incrementaion of the bytes read 
    bytes_read += read_bytes;
    
  }

  //If the process has not return a failure, return that the process was successful
  return true;
  
}

/* attempts to write n bytes to fd; returns true on success and false on
 * failure */
static bool nwrite(int fd, int len, uint8_t *buf) {

  //A tracker that keeps track of the number of bytes that have been written
  int bytes_write = 0;

  //While the length is greater than the bytes written, run the loop
  while(len > bytes_write){

    //Writes the bytes
    int write_bytes = write(fd, &buf[bytes_write], (len - bytes_write));

    //If the write bytes is less than or equal to zero, return that the process was a failure
    if(write_bytes <= 0)
      return false;

    //The incrementation of the bytes written
    bytes_write += write_bytes;
      
  }

  //If the process has not return a failure, return that the process was successful
  return true;
  
}

/* attempts to receive a packet from fd; returns true on success and false on
 * failure */
static bool recv_packet(int fd, uint32_t *op, uint16_t *ret, uint8_t *block) {

  //Variables for recv_pakcet
  uint8_t pack[HEADER_LEN];
  int math = 256 + HEADER_LEN;
  uint16_t len;

  //If it fails to read, return that the process was a failure
  if(nread(fd, HEADER_LEN, pack) == false)
    return false;

  //If it reads successfully
  else{
    
    //Below code updates and copys information for the packet
    //Copys the information of length, info code, and opcode
    memcpy(&len, pack, 2);
    memcpy(ret, &pack[6], 2);
    memcpy(op, &pack[2], 4);

    //Updates the length, info code, and opcode
    len = htons(len);
    *ret = htons(*ret);
    *op = htonl(*op);
    
  }

  //If the size of a block plus the header is equal to the length
  if(math == len){

    //If it fails to read, return that the process was a failure
    if(nread(fd, 256, block) == false)
      return false;
  }

  //Return that the process was a success
    else
      return true;
    
  //Return that the process was a success
  return true;
  
}

/* attempts to send a packet to sd; returns true on success and false on
 * failure */
static bool send_packet(int sd, uint32_t op, uint8_t *block) {

  //Below are variables for the send packet
  int math = 256 + HEADER_LEN;
  uint8_t pack[math];

  //Translate the op code
  int hold = (op >> 26);
  uint32_t networkop = htonl(op);

  //Update the pack with the opcode
  memcpy(&pack[2], &networkop, 4);

  //Translate the length
  uint16_t len = HEADER_LEN;
  uint16_t networklen = htons(len);  

  //If the shifted bytes is equal to the jbod write block
  if(hold == JBOD_WRITE_BLOCK){

    //Update the information of the pack to the block
    memcpy(&pack[8], block, 256);

    //Add a block to the length and translate the length
    len = math;
    networklen = htons(len);
  }

  //Update the information from the pack with the length
  memcpy(pack, &networklen, 2);

  //If it fails to write, return that the process was a failure
  if(nwrite(sd, len, pack) == false)
    return false;

  //Return that the process was a success
  else
    return true;

  //Return that the process was a success
  return true;
  
}

/* attempts to connect to server and set the global cli_sd variable to the
 * socket; returns true if successful and false if not. */
bool jbod_connect(const char *ip, uint16_t port) {
  
  //This creates the socket 
  cli_sd = socket(AF_INET, SOCK_STREAM, 0);

  //The address for the server
  struct sockaddr_in hold;

  hold.sin_port = htons(port);
  
  hold.sin_family = AF_INET;

  //Failure cases
  //If the process can not connect, return that the connection has failed
  if(connect(cli_sd, (const struct sockaddr*) &hold, sizeof(hold)) == -1)
    return false;

  //If the creation of a socket is a failure, retrun that the connection has failed
  if(cli_sd == -1)
    return false;
  
  //If a string can not be converted to a binary string, return that the connection has failed
  if(inet_aton(ip, &hold.sin_addr) == 0)
    return false;

  //If the process has not return a failure from the failure cases, return that the connection was successful
  return true;

  
}

/* disconnects from the server and resets cli_sd */
void jbod_disconnect(void) {

  //Disconnects you from the server
  close(cli_sd);

  //Resests the Cli_sd 
  cli_sd = -1;
  
}

/* sends the JBOD operation to the server and receives and processes the
 * response. */
int jbod_client_operation(uint32_t op, uint8_t *block) {

  //Variable for the reading the recieved packet
  uint16_t ret;
  
  //This writes the packet
  send_packet(cli_sd, op, block);

  //This reads the recieved packets response
  recv_packet(cli_sd, &op, &ret, block);
  return ret;
  
}
