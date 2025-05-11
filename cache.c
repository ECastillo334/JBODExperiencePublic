#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cache.h"

static cache_entry_t *cache = NULL;
static int cache_size = 0;
static int clock = 0;
static int num_queries = 0;
static int num_hits = 0;

int cache_create(int num_entries) { //Creates the cache and sets each enty to valikd = false and disk number, block number, and access time  to 0/ 
  if(cache_enabled() || num_entries < 2 || num_entries > 4096){
    return -1;
  }
  else{
    cache_size = num_entries;
    //Create a cache with cache:
    cache = malloc(cache_size*sizeof(cache_entry_t));
    for(int i = 0; i < cache_size; i++){
      cache[i].valid = false;
      cache[i].disk_num = -1;
      cache[i].block_num = -1;
      cache[i].access_time = 0;
    }
    //DEBUG:
    //printf("cache create succedded\n");
    return 1;
  }
}

int cache_destroy(void) { //Frees up space allocated to the cache.
  if(cache_enabled() == false){
    return -1;
  }
  else{
    free(cache);
    cache = NULL;
    cache_size = 0;
    //return 1;
  }
  return 1;
}

int cache_lookup(int disk_num, int block_num, uint8_t *buf) { //lookups a value equal to the disk and block inputs.
  int feedback = -1;
  if (cache_enabled() == false || buf == NULL || disk_num < 0 || disk_num > 16 || block_num < 0 || block_num > 255){
    return feedback;
  }
  for (int i = 0; i < cache_size; i++){
    if (cache[i].valid == true && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
      memcpy(buf, cache[i].block, 256);
      num_hits += 1; //increase on success
      clock += 1; //increase on success
      cache[i].access_time = clock;
      feedback = 1;   //Change feedback to success
    }
  }
  num_queries += 1; //always increase
  //DEBUG:
  //printf("cache lookup succeded");
  return feedback;
}

void cache_update(int disk_num, int block_num, const uint8_t *buf) { //used for write to block
  //Used to update an entry's block that is already in the cache with buf.
  if (cache_enabled()){
    if (disk_num >= 0 && disk_num <= 15 && block_num >= 0 && block_num <= 255){
      for (int i = 0; i < cache_size; i++){
	if (cache[i].valid == true && cache[i].disk_num == disk_num && cache[i].block_num == block_num){
	  memcpy(cache[i].block, buf, 256);
	  clock += 1;
	  cache[i].access_time = clock;
	  //DEBUG:
	  //printf("cache update succeded");
	}
      }
    }
  }
}

int cache_insert(int disk_num, int block_num, const uint8_t *buf) { // Used to add a new value to the cache.
  //LRU will be used
  int index = 0;
  int lowest_time = -1;
  if (buf == NULL  || cache_enabled() == false || disk_num < 0 || disk_num > 16 || block_num < 0 || block_num > 255){
    //printf("%d %d \n", disk_num, block_num);
    //printf("Out of range\n");
    return -1;
  }
  for(int i = 0; i < cache_size; i++){
    if (cache[i].disk_num == disk_num && cache[i].block_num == block_num && (memcmp(cache[i].block, buf, 256) == 0)){
      //printf("It is already in this");
      return -1;
    }
    else if(lowest_time == -1 || lowest_time > cache[i].access_time){
      index = i;
      lowest_time = cache[i].access_time;
    }
  }
  cache[index].valid = true;
  clock += 1;
  cache[index].access_time = clock;
  cache[index].disk_num = disk_num;
  cache[index].block_num = block_num;
  memcpy(cache[index].block, buf, 256);
  //DEBUG:
  //printf("cache insert  succeded \n  ");
  return 1;
}

bool cache_enabled(void) { //Tests to see if a cache is enabled
  //Replaces a global variable
  //Don't know if I use this in read/write yet.
  if (cache_size >= 2 && cache_size <= 4096){
    return true;
  }
  else{
    return false;
  }
}

void cache_print_hit_rate(void) {
  fprintf(stderr, "Hit rate: %5.1f%%\n", 100 * (float) num_hits / num_queries);
}
