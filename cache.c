#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) {

  //If 2 entries is less than the number of entries return that the process has failed
  if(2 > num_entries)
    return -1;

  //If 4096 entries is more than the number of entries return that the process has failed
  if(4096 < num_entries)
    return -1;

  //If the cache has been enabled already return that the process has failed
  if(cache_enabled())
    return -1;

  //Use calloc to dynamically allocate space for the number of entries that are in the cache
  cache_size = num_entries;
  cache = calloc(cache_size, sizeof(cache_entry_t));

  //Return that the process has been successful
  return 1;
  
}

int cache_destroy(void) {

  //If the cache has been disabled already return that the process has failed
  if(cache_enabled() == false)
    return -1;

  //Free allocated memory and set the cache to NULL
  free(cache);
  cache = NULL;

  //Return that the process was successful
  return 1;
  
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) {

  //If but is NUll (empty) return that the process has failed
  if(buf == NULL)
    return -1;
  
  //If the cache has been disabled return that the process has failed
  if(cache_enabled() == false)
    return -1;

  //Loops through the cache
  for(int hold = 0; hold < cache_size; hold++){

    //If you can find the block
    if(cache[hold].block_num == block_num && cache[hold].disk_num == disk_num
       && cache[hold].valid){

      //Record the time of access
      cache[hold].access_time = clock;

      //Incrementation of clock by one
      clock += 1;

      //Copy the blocks information into buf
      memcpy(buf, cache[hold].block, JBOD_BLOCK_SIZE);

      //Incrementation of number of hits and number of queries by one
      num_hits += 1;
      num_queries += 1;

      //Return that the process was successful
      return 1;
    }
  }

  //Incrementation of clock and the number of queries by one
  clock += 1;
  num_queries += 1;

  //return that the process has failed
  return -1;
  
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) {

  //Loops through the cache
  for(int hold = 0; hold < cache_size; hold++){

    //If you can find the block
    if(cache[hold].block_num == block_num && cache[hold].disk_num == disk_num
       && cache[hold].valid){

      //Record the time of access
      cache[hold].access_time = clock;

      //Incrementation of clock by one
      clock += 1;

      //Copys the blocks information into buf
      memcpy(cache[hold].block, buf, JBOD_BLOCK_SIZE);

      break;
      
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) {

  //If you try to insert an invalid block return that the process has failed
  if(0 > block_num)
    return -1;

  //If you try to insert an invalid block return that the process has failed
  if(JBOD_NUM_BLOCKS_PER_DISK < block_num)
    return -1;

  //If you try to insert an invalid disk return that the process has failed
  if(0 > disk_num)
    return -1;

  //If you try to insert an invalid disk return that the process has failed
  if(JBOD_NUM_DISKS < disk_num)
    return -1;

  //If but is NUll (empty) return that the process has failed
  if(buf == NULL)
    return -1;

  //If the cache has been disabled return that the process has failed
  if(cache_enabled() == false)
    return -1;

  uint8_t myBuf[256];

  //If you try to insert an entry that already exist return that the process has failed
  if(cache_lookup(disk_num, block_num, myBuf) == 1)
    return -1;

  //Variable for below for loop
  int cache_access = cache[0].access_time;

  //A for loop
  for(int hold = 0; hold < cache_size; hold++){

    //Variable for below if statement
    int cache_hold_t = cache[hold].access_time;

    //If the cache is is not valid (false)
    if(cache[hold].valid == false){

      //Record the access time
      cache[hold].access_time = clock;

      //Incrementation of clock by one
      clock += 1;

      //Sets the cache to valid (true)
      cache[hold].valid = true;

      //Copys the blocks information into buf
      memcpy(cache[hold].block, buf, JBOD_BLOCK_SIZE);

      //Find the block and disk number and insert a new entry
      cache[hold].block_num = block_num;
      cache[hold].disk_num = disk_num;

      //Return that the process was successful
      return 1; 
    }
    
    //If the recorded access time is less than the last recorded access time,
    //replace the smallest access time
    if(cache_access > cache_hold_t)
       cache_access = cache_hold_t;
  }

  //A for loop
  for(int hold = 0; hold < cache_size; hold++){

    //If the recorded access time is equal to the smallest recorded access time
    if(cache[hold].access_time == cache_access){

      //Record the access time
      cache[hold].access_time = clock;

      //Incrementation of clock by one
      clock += 1;

      //Copys the blocks information into buf
      memcpy(cache[hold].block, buf, JBOD_BLOCK_SIZE);

      //Find the block and disk number and insert a new entry
      cache[hold].block_num = block_num;
      cache[hold].disk_num = disk_num;

      //Return that the process was successful
      return 1;
      
    }
  }
  
  //Return that the process has failed
  return -1;
  
}

bool cache_enabled(void) {

  //If the cache is not equal to NULL (empty) return that the cache has been enabled
  if(cache != NULL)
    return true;

  //Returns that the cache has been disabled
  return false;
  
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
