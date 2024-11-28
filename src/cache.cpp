// Sample cache implementation with a random variable to simulate hits and misses

#include "cache.h"

#include <cmath>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>
#include <random>

#include "Utilities.h"

using namespace std;

// TODO: Modify this file to complete your implementation of the cache

// Random generator for cache hit/miss simulation
static std::mt19937 generator(42); // Fixed seed for deterministic results
std::uniform_real_distribution<double> distribution(0.0, 1.0);

// Constructor definition
Cache::Cache(CacheConfig configParam, CacheDataType cacheType) : config(configParam)
{
    // Here you can initialize other cache-specific attributes
    // For instance, if you had cache tables or other structures, initialize them here
    hits = 0;
    misses = 0;

    this->cacheType = cacheType;
    cacheSize = configParam.cacheSize;
    blockSize = configParam.blockSize;
    ways = configParam.ways;
    missLatency = configParam.missLatency;
    numSets = config.cacheSize / (blockSize * ways);
    
    blockOffset = (uint32_t)log2((double)blockSize / 4);
    indexBits = (uint32_t)log2((double)numSets);
    cache.resize(numSets, vector<CacheEntry>(ways));
    lru.resize(numSets, vector<uint32_t>(ways));

}

// Access method definition
bool Cache::access(uint32_t address, CacheOperation readWrite)
{
    // For simplicity, we're using a random boolean to simulate cache hit/miss
    // bool hit = distribution(generator) < 0.20;  // random 20% hit for a strange cache
    // hits += hit;
    // misses += !hit;

    uint32_t index = getIndex(address);
    uint32_t tag = getTag(address);

    // first update LRU values
    LRUincr(index);

    // now check if exists
    for (uint32_t i = 0; i < ways; i++)
    {
        if (cache[index][i].valid && cache[index][i].tag == tag)
        {
            lru[index][i] = 0;  // Reset LRU for this block
            hits++;
            return true;
        }
    }

    misses++;
    
    uint32_t replaceIndex = findReplacementBlock(index);
    cache[index][replaceIndex].tag = tag;
    cache[index][replaceIndex].valid = true;
    lru[index][replaceIndex] = 0;

    return false;
}

void Cache::LRUincr(uint32_t index)
{    
    // Increment LRU all blocks
    for (uint32_t i = 0; i < ways; i++)
    {
        lru[index][i]++;
    }
}

uint32_t Cache::findReplacementBlock(uint32_t index)
{    
    uint32_t maxLRU = 0;
    uint32_t replacementIndex = 0;
    
    for (uint32_t i = 0; i < ways; i++)
    {
        if (lru[index][i] > maxLRU)
        {
            maxLRU = lru[index][i];
            replacementIndex = i;
        }
    }
    
    return replacementIndex;
}

// Dump method definition, you can write your own dump info
Status Cache::dump(const std::string &base_output_name)
{
    ofstream cache_out(base_output_name + "_cache_state.out");
    // dumpRegisterStateInternal(reg, cache_out);
    if (cache_out)
    {
        cache_out << "---------------------" << endl;
        cache_out << "Begin Register Values" << endl;
        cache_out << "---------------------" << endl;
        cache_out << "Cache Configuration:" << std::endl;
        cache_out << "Size: " << config.cacheSize << " bytes" << std::endl;
        cache_out << "Block Size: " << config.blockSize << " bytes" << std::endl;
        cache_out << "Ways: " << (config.ways == 1) << std::endl;
        cache_out << "Miss Latency: " << config.missLatency << " cycles" << std::endl;
        cache_out << "---------------------" << endl;
        cache_out << "End Register Values" << endl;
        cache_out << "---------------------" << endl;
        return SUCCESS;
    }
    else
    {
        cerr << LOG_ERROR << "Could not create cache state dump file" << endl;
        return ERROR;
    }

    // Here you can also dump the state of the cache, its stats, or any other relevant information
}
