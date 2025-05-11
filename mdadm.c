/* Author: Ellyot Castillo
   Date: 4/30/25
*/
    
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "cache.h"
#include "mdadm.h"
#include "util.h"
#include "jbod.h"
#include "net.h"

int mounted = 0; //Global variable to keep track of mounted status
//uint8_t OtherBuf[256];

int mdadm_mount(void) { //Initializes our system
  /* YOUR CODE */
  int feedback = -1;
  if (mounted == 0){
    //feedback = JBOD_MOUNT;
    //feedback = jbod_operation(JBOD_MOUNT, NULL);
    feedback = jbod_client_operation(JBOD_MOUNT, NULL);
    mounted = 1;
  }
  else{
    return -1;
  }
  if (feedback == 0){
    return 1;
  }
  else{
    return -1;
  }
}

int mdadm_unmount(void) { //stops all operations
  /* YOUR CODE */
  int feedback = -1;
  if (mounted == 1){
    mounted = 0;
    //feedback = jbod_operation(JBOD_UNMOUNT, NULL);
    feedback = jbod_client_operation(JBOD_UNMOUNT, NULL);
  }
  else{
    return -1;
  }

  if (feedback == 0){
    return 1;
  }
  else{
    return -1;
  }
}

int mdadm_read(uint32_t addr, uint32_t len, uint8_t *buf) { //Reads data
  /* YOUR CODE */
  //Reads len digits into the disk and block that start at addr and saves into buf.
  // Can read up to 1024 bytes
  //May need to use helper function to go to the correct disk and then read it. Did not need it.
  //cache_create(1024);
  //printf("Reading now\n");
  int feedback = 0;
  int op = 0;
  int ret = 0;
  if (mounted == 0  || len > 1024 || (addr + len) > (256*256*16) ){
    //printf("1 of 3 problems\n");   //DEBUG
    return -1;
  }
  else if(buf == NULL && len != 0){
    //printf("NULL and not 0 happened\n");   //DEBUG
    return -1;
  }
  else if (buf == NULL && len == 0){
    //printf("NULL and 0 happened\n");  //DEBUG
    return 0;
  }
  else{  //Main code that reads the data.
    int disk = addr/(256*256);
    int block = ((addr%65536)/256); 
    int bits = addr%256;
    uint8_t OtherBuf[256];
    int BytesRead = 0;
    while (len > 0){   //Runs until it has read len digits.
      /*printf("While loop went\n"); //DEBUG
      printf("feed: %d \n", feedback);  //DEBUG
      printf("len: %d \n", len);   //DEBUG
      printf("block: %d \n", block);  //DEBUG
      printf("bit: %d \n", bits);   //DEBUG
      printf("disk: %d \n", disk);*/  //DEBUG

      if(cache_enabled()){
      	//printf("Cache is actually enabled");
	if(cache_lookup(disk, block, OtherBuf) == -1){ //Addition of cache
	  op = JBOD_SEEK_TO_DISK |(disk << 20);  
	  //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	   //printf("Failed with disk1");  //DEBUG
	    return -1;
	  }
	  op = JBOD_SEEK_TO_BLOCK | (block << 24);
	  //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	    //printf("Failed with blocks1");  //DEBUG
	    return -1;
	  }
	  //ret = jbod_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	  ret = jbod_client_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
      
	  //printf("OtherBuf: %p\n", OtherBuf);  //DEBUG
	  if (ret == -1){
	    //printf("Failed with read1");  //DEBUG
	    return -1;
	  }
	
	  if(cache_enabled() == false){
	    //printf("Cache is disabled");
	    //return -1;
	  } 
	  ret = cache_insert(disk, block, OtherBuf); //Problem Area
	  if (ret == -1 && cache_enabled()){
	    //printf("cache insert failed Reading"); //DEBUG
	    return -1;
	  }
	}
      }
      else{
	op = JBOD_SEEK_TO_DISK |(disk << 20);  
	//ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
	if (ret == -1){
          //printf("Failed with disk2");  //DEBUG
          return -1;
        }
        op = JBOD_SEEK_TO_BLOCK | (block << 24);
        //ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
        if (ret == -1){
          //printf("Failed with blocks2");  //DEBUG
          return -1;
        }
        //ret = jbod_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	ret = jbod_client_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	//printf("OtherBuf: %p\n", OtherBuf);  //DEBUG
        if (ret == -1){
          return -1;
        }
      }
     
      if (len<(256-bits)){
	//feedback += len;
	//memcpy(buf,OtherBuf+bits,len);
        BytesRead = len;
	//printf("Condition1 bytes: %d\n", BytesRead);  //DEBUG
      }
      else{
	//feedback += (256-bits);
	//memcpy(buf,OtherBuf+bits,(256-bits));
        BytesRead = (256-bits);
	//printf("Condition2 bytes: %d\n", BytesRead);  //DEBUG
      }
      /*printf("Buf: %p\n", buf);  //DEBUG
      printf("feedback before memcpy: %d\n", feedback);  //DEBUG
      printf("bits before memcpy: %d\n", bits);*/  //DEBUG
      memcpy(buf+feedback,OtherBuf+bits,BytesRead);  
      feedback += BytesRead;
      len -= BytesRead;
      
      if (len > 0){
        bits = 0;
        block += 1;
        if (block >= 255){
          block = 0;
          disk += 1;
          op = JBOD_SEEK_TO_DISK | (disk << 20);
          //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	    printf("Failed with disk3");  //DEBUG
	    return -1;
	  }
	  //op = JBOD_SEEK_TO_BLOCK | (block << 24);
	  //jbod_operation((op-3), NULL);
	}
      }
      /*printf("feed: %d \n", feedback);  //DEBUG
      printf("len: %d \n", len);   //DEBUG
      printf("block: %d \n", block);  //DEBUG
      printf("bit: %d \n", bits);   //DEBUG
      printf("disk: %d \n", disk);*/  //DEBUG
    }
  }
  //cache_destroy();
  return feedback;
}



int mdadm_write(uint32_t addr, uint32_t len, const uint8_t *buf) { //Writes to the specified disk, block, and byte
  //I should read before writing. Do this to prevent the info you aren't writing over from disapearing.
  //writes len bytes from buf into storage starting at addr
  //Need to read the whole block then combine only what im writing with what else needs to be read, then JBOD_WRITE_BLOCK(op, buf)
  //Put all data I'm writing into OtherBuf

  //cache_create(1024);
  int BytesRead = 0;
  if (mounted == 0  || len > 1024 || (addr + len) > (256*256*16) ){
    //printf("1 of 3 problems\n");   //DEBUG
    return -1;
  }
  else if(buf == NULL && len != 0){
    //printf("NULL and not 0 happened\n");   //DEBUG
    return -1;
  }
  else{
    //Code goes here
    int disk = addr/(256*256);
    int block = ((addr%65536)/256); 
    int bits = addr%256;
    uint8_t CombinedBuf[256];
    uint8_t OtherBuf[256];
    int op = 0;
    int ret = 0;

    while (len > 0){   //Real part of the code begins here
      if(cache_enabled()){
	if (cache_lookup(disk, block, OtherBuf) == -1){
	  op = JBOD_SEEK_TO_DISK |(disk << 20);  
	  //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	    //printf("Failed with disk1");  //DEBUG
	    return -1;
	  }
	  op = JBOD_SEEK_TO_BLOCK | (block << 24);
	  //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	    //printf("Failed with blocks1");  //DEBUG
	    return -1;
	  }
	  //ret = jbod_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	  ret = jbod_client_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	  if (ret == -1){
	    return -1;
	  }
	  ret = cache_insert(disk, block, OtherBuf);
	  if (ret == -1 && cache_enabled()){
	    //printf("cache insert failed"); //DEBUG
	    return -1;
	  }
	}
      }

      else{
	op = JBOD_SEEK_TO_DISK |(disk << 20);  
	//ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
        if (ret == -1){
          //printf("Failed with disk2");  //DEBUG
          return -1;
        }
        op = JBOD_SEEK_TO_BLOCK | (block << 24);
	//ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
	if (ret == -1){
	  //printf("Failed with blocks2");  //DEBUG
	  return -1;
        }
        //ret = jbod_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
	ret = jbod_client_operation(JBOD_READ_BLOCK, (uint8_t *)OtherBuf);
        if (ret == -1){
	  return -1;
	}
      }
	//Done reading
	//uint8_t CombinedBuf[256];
	op = JBOD_SEEK_TO_DISK |(disk << 20);
        //ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
        if (ret == -1){
          //printf("Failed with disk3");  //DEBUG
          return -1;
        }
        op = JBOD_SEEK_TO_BLOCK | (block << 24);
        //ret = jbod_operation(op, NULL);
	ret = jbod_client_operation(op, NULL);
	if (ret == -1){
          //printf("Failed with blocks3");  //DEBUG
          return -1;
        }
	
      int offset = 256-bits;
      if (offset > len){
        offset = len;
      }
      
      BytesRead += offset;
      len -= offset;
      memcpy(CombinedBuf, OtherBuf, 256);
      memcpy(&CombinedBuf[bits], &buf[BytesRead - offset], offset);
    
      //jbod_operation(JBOD_WRITE_BLOCK, (uint8_t *)CombinedBuf);
      jbod_client_operation(JBOD_WRITE_BLOCK, (uint8_t *)CombinedBuf);

      if (cache_enabled()){
	cache_update(disk, block, CombinedBuf);   //Cache Update
      }
      
      if (len>0){
	bits = 0;
	block += 1;
	if (block >= 256){
	  block = 0;
	  disk += 1;
	  if (disk >= 16){
	    disk = 0;
	  }
	  op = JBOD_SEEK_TO_DISK |(disk << 20);  
	  //ret = jbod_operation(op, NULL);
	  ret = jbod_client_operation(op, NULL);
	  if (ret == -1){
	    //printf("Failed with disk4");  //DEBUG
	    return -1;
	  }
	  /*op = JBOD_SEEK_TO_BLOCK | (block << 24);
	  ret = jbod_operation(op, NULL);
	  if (ret == -1){
	    //printf("Failed with blocks");  //DEBUG
	    return -1;
	    }*/
	}
      }
    }
  }
  //cache_destroy();
  return BytesRead;
}
