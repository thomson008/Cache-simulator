/***************************************************************************
 * *    Inf2C-CS Coursework 2: Cache Simulation
 * *
 * *    Instructor: Boris Grot
 * *
 * *    TA: Siavash Katebzadeh
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
/* Do not add any more header files */

/*
 * Various structures
 */
typedef enum {FIFO, LRU, Random} replacement_p;

const char* get_replacement_policy(uint32_t p) {
    switch(p) {
    case FIFO: return "FIFO";
    case LRU: return "LRU";
    case Random: return "Random";
    default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
} mem_access_t;

// These are statistics for the cache and should be maintained by you.
typedef struct {
    uint32_t cache_hits;
    uint32_t cache_misses;
} result_t;


/*
 * Parameters for the cache that will be populated by the provided code skeleton.
 */

replacement_p replacement_policy = FIFO;
uint32_t associativity = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;


/*
 * Each of the variables below must be populated by you.
 */
uint32_t g_num_cache_tag_bits = 0;
uint32_t g_cache_offset_bits= 0;
result_t g_result;


/* Reads a memory access from the trace file and returns
 * 32-bit physical memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!= NULL) {
        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtoul(token, NULL, 16);
        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

void print_statistics(uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {
    /* Do Not Modify This Function */

    uint32_t cache_total_hits = r->cache_hits;
    uint32_t cache_total_misses = r->cache_misses;
    printf("CacheTagBits:%u\n", num_cache_tag_bits);
    printf("CacheOffsetBits:%u\n", cache_offset_bits);
    printf("Cache:hits:%u\n", r->cache_hits);
    printf("Cache:misses:%u\n", r->cache_misses);
    printf("Cache:hit-rate:%2.1f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
}

/*
 *
 * Add any global variables and/or functions here as needed.
 *
 */

uint32_t g_cache_index_bits= 0;

uint32_t getOffsetBits(uint32_t block_size) {
  uint32_t bits = 0, i;
  uint32_t highestOffset = block_size - 1;

  for (i = highestOffset; i != 0; i>>=1) {
    bits++;
  }
	return bits;
}

uint32_t getIndexBits(uint32_t blocks_number, uint32_t associativity) {
  uint32_t number_of_sets = blocks_number / associativity;
  uint32_t result = 0;
  uint32_t current_power = 1;

  while (current_power < number_of_sets) {
    current_power *= 2;
    result++;
  }

  return result;
}

uint32_t getTagBits(uint32_t offset, uint32_t index) {
  return 32 - offset - index;
}

uint32_t getTag(mem_access_t access) {
  return access.address >> (g_cache_offset_bits + g_cache_index_bits);
}

uint32_t getIndex(mem_access_t access) {
  uint32_t address = access.address;
  return (((1 << g_cache_index_bits) - 1) & (address >> (g_cache_offset_bits)));
}

int found;
int* FIFO_loc = (int*) malloc(n_of_sets * sizeof(int));
uint32_t n_of_sets;
uint32_t n_of_blocks;

void fifoPolicy(uint32_t tag, uint32_t index, Set theSet) {
  found = 0;

  int where = FIFO_loc[index];

  for (i = 0; i < n_of_blocks; i++) {
    if (theSet.blocks[i].valid_bit) {                                   //check if the block is used
      if (theSet.blocks[i].tag == tag) {                                //see if the tags match
        found++;                                                        //if they match, set found to 1
        g_result.cache_hits++;                                          //update statistic for hits
        break;
      }
    }
  }

  if (found == 0) {                                                     //if matching tag wasn't found:
    theSet.blocks[where].tag = tag;                                     //put the tag in a block
    theSet.blocks[where].valid_bit = 1;                                 //update the valid bit so it is known that the block is used
    where = (where + 1) % n_of_blocks;                                  //calculate the location for next tag
    g_result.cache_misses++;                                            //update stats for misses
    FIFO_loc[index] = where;                                            //put the new location in the appropriate index in locations array
  }
}

void randomPolicy(uint32_t tag, uint32_t index, Set theSet) {
  found = 0;

  for (i = 0; i < n_of_blocks; i++) {
    if (theSet.blocks[i].valid_bit) {
      if (theSet.blocks[i].tag == tag) {
        found++;
        g_result.cache_hits++;
        break;
      }
    }
  }

  if (found == 0) {
    g_result.cache_misses++;
    int found_empty = 0;
    for (i = 0; i < n_of_blocks; i++) {                                 //in case of a miss, check if there is any empty block available
      if (!theSet.blocks[i].valid_bit) {                                //if yes, put the tag in that block
        theSet.blocks[i].tag = tag;
        theSet.blocks[i].valid_bit = 1;                                 //set valid bit to 1 for that block
        found_empty++;                                                  //indicate that empty block was found
        break;
      }
    }
    if (!found_empty) {                                                 //if it wasn't found
        int where = rand() % n_of_blocks;                               //randomly choose a block to replace
        theSet.blocks[where].tag = tag;                                 //put the tag in there
    }
  }
}

void LRUPolicy(uint32_t tag, uint32_t index, Set theSet) {
  for (i = 0; i < n_of_blocks; i++) {
    if (theSet.blocks[i].valid_bit) {
      if (theSet.blocks[i].tag == tag) {
        found++;
        g_result.cache_hits++;
        for (j = 0; j < n_of_blocks; j++) {                             //update the access_time for all blocks that have been already accessed
          if (theSet.blocks[j].valid_bit) theSet.blocks[j].access_time++;
        }
        theSet.blocks[i].access_time = 0;                               //set access time for the current block
        break;
      }
    }
  }

  if (found == 0) {                                                     //in case of a miss
    g_result.cache_misses++;                                            //update miss counter
    int found_empty = 0;
    for (i = 0; i < n_of_blocks; i++) {                                 //look for empty block
      if(!theSet.blocks[i].valid_bit) {
        theSet.blocks[i].tag = tag;                                     //if found, put the tag there
        theSet.blocks[i].valid_bit = 1;
        for (j = 0; j < n_of_blocks; j++) {                             //update access_time for all used blocks
          if (theSet.blocks[j].valid_bit)
            theSet.blocks[j].access_time++;
        }
        theSet.blocks[i].access_time = 0;                               //set access_time for the current block to 0
        found_empty++;                                                  //indicate that an empty block was found
        break;
      }
    }

    if (found_empty == 0) {                                             //if it wasn't, find the block to replace
      uint32_t max_access = 0;
      uint32_t oldestIdx = 0;
      for (i = 0; i < n_of_blocks; i++) {                               //find the max access time
        if (theSet.blocks[i].access_time > max_access) {
          max_access = theSet.blocks[i].access_time;
          oldestIdx = i;
        }
      }

      for (j = 0; j < n_of_blocks; j++) {                               //update access_time for all used blocks
        if (theSet.blocks[j].valid_bit)
          theSet.blocks[j].access_time++;
      }

      theSet.blocks[oldestIdx].tag = tag;                               //put the tag in a selected block
      theSet.blocks[oldestIdx].access_time = 0;                         //set it as most recently used
    }
  }
}


int main(int argc, char** argv) {
    time_t t;
    /* Intializes random number generator */
    /* Important: *DO NOT* call this function anywhere else. */
    srand((unsigned) time(&t));
    /* ----------------------------------------------------- */
    /* ----------------------------------------------------- */

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if (argc < 6) {
        improper_args = 1;
        printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if (strcmp(argv[1], "FIFO") == 0) {
            replacement_policy = FIFO;
        } else if (strcmp(argv[1], "LRU") == 0) {
            replacement_policy = LRU;
        } else if (strcmp(argv[1], "Random") == 0) {
            replacement_policy = Random;
        } else {
            improper_args = 1;
            printf("Usage: ./mem_sim [replacement_policy: FIFO LRU Random] [associativity: 1 2 4 8 ...] [number_of_cache_blocks: 16 64 256 1024] [cache_block_size: 32 64] mem_trace.txt\n");
        }
        associativity = atoi(argv[2]);
        number_of_cache_blocks = atoi(argv[3]);
        cache_block_size = atoi(argv[4]);
        strcpy(file, argv[5]);
    }
    if (improper_args) {
        exit(-1);
    }
    assert(number_of_cache_blocks == 16 || number_of_cache_blocks == 64 || number_of_cache_blocks == 256 || number_of_cache_blocks == 1024);
    assert(cache_block_size == 32 || cache_block_size == 64);
    assert(number_of_cache_blocks >= associativity);
    assert(associativity >= 1);

    printf("input:trace_file: %s\n", file);
    printf("input:replacement_policy: %s\n", get_replacement_policy(replacement_policy));
    printf("input:associativity: %u\n", associativity);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file = fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));

    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */

    /* You may want to setup your Cache structure here. */

    //structure for a single cache block
    typedef struct {
      uint32_t tag;
      int valid_bit;
      uint32_t access_time;
    } Block;

    //structure for a set of cache, holding an array of blocks
    typedef struct {
      Block* blocks;
    } Set;

    //structure for the cache itself, holding an array of sets;
    typedef struct {
      Set*  sets;
    } Cache;

    n_of_sets = number_of_cache_blocks / associativity;
    n_of_blocks = associativity;

    //dynamic allocation of space for an array of sets
    Cache cache;
    cache.sets = (Set*) malloc(n_of_sets * sizeof(Set));

    uint32_t i;
    uint32_t j;

    //dynamic allocation of space for an arrray of blocks in every set
    for (i = 0; i < n_of_sets; i++) {
      cache.sets[i].blocks = (Block*) malloc(n_of_blocks * sizeof(Block));
    }

    //initializing the valid bits in every block to 0 - none of them is in use yet
    for (i = 0; i < n_of_sets; i++) {
      for (j = 0; j < n_of_blocks; j++) {
        cache.sets[i].blocks[j].valid_bit = 0;
        cache.sets[i].blocks[j].access_time = 0;
      }
    }

    //calculating numbers of tag, index and offset bits
    g_cache_index_bits = getIndexBits(number_of_cache_blocks, associativity);
    g_cache_offset_bits = getOffsetBits(cache_block_size);
    g_num_cache_tag_bits = getTagBits(g_cache_offset_bits, g_cache_index_bits);

    for (i = 0; i < n_of_sets; i++) {
      FIFO_loc[i] = 0;
    }

    mem_access_t access;
    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;

        /* Add your code here */
        uint32_t tag = getTag(access);
        uint32_t index = getIndex(access);
        Set theSet = cache.sets[index];

        switch(replacement_policy) {
          case FIFO:
            fifoPolicy(tag, index, theSet);
          case Random:
            randomPolicy(tag, index, theSet);
          case LRU:
            LRUPolicy(tag, index, theSet);
        }
    }

    for (i = 0; i < n_of_sets; i++) {
      free(cache.sets[i].blocks);
    }

    free(cache.sets);


    /* Do not modify code below. */
    /* Make sure that all the parameters are appropriately populated. */
    print_statistics(g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;
}
