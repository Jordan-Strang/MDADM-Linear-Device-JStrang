#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "mdadm.h"
#include "jbod.h"

//Helper function, Code given during lecture
uint32_t encode_op(int cmd, int disk_num, int block_num){
  
  //A 32 bit integer type
  uint32_t op = 0;

  //|= is a bitwise or assingment, << Shifts bits to the left 
  op |= (cmd << 26);
  op |= (disk_num << 22);
  op |= (block_num);

  return op;
  
}

//This is a global variable that is used to chechk if the disks are mounted or unmounted
int mounted_or_unmounted = 0;

//The function to Mount all of the disks
int mdadm_mount(void) {

  //If the disk is already Mounted return that the process has failed
  if(mounted_or_unmounted == 1)
    return -1;
  
  //If the disk is Unmounted change the global variable to say that it is Mounted 
  if(mounted_or_unmounted == 0)
    mounted_or_unmounted = 1;
  
  //Code given during lecture
  uint32_t op = encode_op(JBOD_MOUNT, 0, 0);
  jbod_client_operation(op, NULL);

  //returns that the function is succesful (disks have been Mounted)
  return 1;
  
}

//The function to Unmount all of the disks
int mdadm_unmount(void) {

  //If the disk is already Unmounted return that the process has failed
  if(mounted_or_unmounted == 0)
    return -1;
  
  //If the disk is Mounted change the global variable to say this it is Unmounted
  if(mounted_or_unmounted == 1)
    mounted_or_unmounted = 0;

  //Code given during lecture
  uint32_t op = encode_op(JBOD_UNMOUNT, 0, 0);
  jbod_client_operation(op, NULL);

  //Returns that the function is succesful (disks have been Unmounted)
  return 1;

}

//finds the minimum number, helper function for the while loop in mdadm_read
int min(int a, int b){
  if(a < b)
    return a;
  else
    return b;
  
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) {
  
  //If the disk is Umounted return that the process has failed
  if(mounted_or_unmounted == 0)
    return -1;

  //If you try to read more than 1024 bytes return that the process has failed
  if(1024 < len)
    return -1;

  //If you try to read an out of bounds address return that the process has failed
  if(len + addr > JBOD_NUM_DISKS * JBOD_DISK_SIZE)
    return -1;

  //If the Buffer is equal to NULL and the length is greater than zero return that the process has failed
  if(buf == NULL && len > 0)
    return -1;

  //Variables for the below while loop
  int hold = addr;
  uint8_t mybuf[256];
  int total_block = 0; 

  //Variables to keep track of what is being read
  int tracker_A = len;
  int tracker_B = len; 

  //A while loop
  while (hold < addr + len){

    //Find the disk number
     int disk_num = hold / JBOD_DISK_SIZE;
     
     //Find the block number
     int block_num = (hold % JBOD_DISK_SIZE) / JBOD_BLOCK_SIZE;

     //Find the offset number
     int offset_disk = hold % JBOD_DISK_SIZE;
     int offset_block = offset_disk % JBOD_BLOCK_SIZE;

     //Seeks to disk
     int32_t op_disk_seek_disk_A = encode_op(JBOD_SEEK_TO_DISK, disk_num, block_num);
     jbod_client_operation(op_disk_seek_disk_A, NULL);

     //Seeks to block
     int32_t op_disk_seek_block = encode_op(JBOD_SEEK_TO_BLOCK, disk_num, block_num);
     jbod_client_operation(op_disk_seek_block, NULL);     

     //Lookup and insert into the cache
     if(cache_lookup(disk_num, block_num, buf) == -1){
       
       //Read the block
       jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);

       //Insert into the cache
       cache_insert(disk_num, block_num, buf);
      
     }

     int math = len + offset_block;
     
     //If the total amount of blocks is zero
     if (total_block == 0){

       //If the bytes do not fit into one block
       if(math > 256){

	 //Set tracker_a to 256 subtract the offset block, decrement tracker_b by tracker_a, and increment hold by tracker_a
	 tracker_A = 256 - offset_block; 
	 tracker_B -= tracker_A;
	 hold += tracker_A;

	 //Copies the information from mybuf plus offset block into buf
	 memcpy(buf, mybuf + offset_block, tracker_A);
	 
       }
       
       //If the bytes fit into one block 
       if(math <= 256){

	 //Inrement hold by length and decrement tracker_b by length
	 hold += len;
	 tracker_B -= len;

	 //Copies the information from mybuf plus offset block into buf
	 memcpy(buf, mybuf + offset_block, len);

       }

       //Move away from the first block
       total_block = 1; 
       
     }

     //If tracker_b is less than 256 bytes
     else if(tracker_B < 256){
       
       //Increment hold by tracker_b
       hold += tracker_B;

       //Copies the information from mybuf into buf plus tracker_a
       memcpy(buf + tracker_A, mybuf, tracker_B);
      
     }

     //If tracker_b is more than or equal to 256 bytes
     else if(tracker_B >= 256){

       //increment hold by 256 and buf by tracker_a
       hold += 256;
       buf += tracker_A;

       //Set tracker_a to 256 and decrement tracker_b by 256
       tracker_A = 256;
       tracker_B -= 256;

       //Copies the information from mybuf into buf
       memcpy(buf, mybuf, 256);
       
     }
  }

  //Return the final product of length
  return len;
 
}



int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) {

  //If the disk is Umounted return that the process has failed
  if(mounted_or_unmounted == 0)
    return -1;

  //If you try to read more than 1024 bytes return that the process has failed
  if(1024 < len)
    return -1;

  //If you try to read an out of bounds address return that the process has failed
  if(len + addr > JBOD_NUM_DISKS * JBOD_DISK_SIZE)
    return -1;

  //If the Buffer is equal to NULL and the length is greater than zero return that the process has failed
  if(buf == NULL && len > 0)
    return -1;

  //Variables need for the below while loop
  int hold = addr;
  uint8_t mybuf[256];
  int tracker = 0;

  //while the address is less than the address plus the length
  while(hold < addr + len){
    
    //Find the disk number
    int disk_num = hold / JBOD_DISK_SIZE;

    //Find the block number
    int block_num = hold / JBOD_BLOCK_SIZE;

    //Find the offset disk and offset block number
    int offset_disk = hold % JBOD_DISK_SIZE;
    int offset_block = offset_disk % JBOD_BLOCK_SIZE;

    //Seeks to disk
    int32_t op_disk_seek_disk_A = encode_op(JBOD_SEEK_TO_DISK, disk_num, block_num);
    jbod_client_operation(op_disk_seek_disk_A, NULL);

    //Seeks to block
    int32_t op_disk_seek_block = encode_op(JBOD_SEEK_TO_BLOCK, disk_num, block_num);
    jbod_client_operation(op_disk_seek_block, NULL);

    //Read the block
    jbod_client_operation(encode_op(JBOD_READ_BLOCK, 0, 0), mybuf);

    //Seeks to disk
    jbod_client_operation(op_disk_seek_disk_A, NULL);

    //Seeks to block
    jbod_client_operation(op_disk_seek_block, NULL);   

    //More variables for below if statement
    int math = addr + (len - hold);
    int hold_b = math;
 
    //If the offset is bigger than the disk size
    if(256 < math + offset_block)
      hold_b = 256 - offset_block;

    //Copys the information from the bytes
    memcpy(mybuf + offset_block, buf + tracker, hold_b);   

    //Write the block
    jbod_client_operation(encode_op(JBOD_WRITE_BLOCK, 0, 0), mybuf);

    //Inrementation of tracker, hold, and hold_b
    tracker += hold_b;
    hold += hold_b;
	
    }

  //Returns the final product
  return len;
  
}
