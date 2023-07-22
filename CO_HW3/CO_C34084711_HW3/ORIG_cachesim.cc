// See LICENSE for license details.

#include "cachesim.h"
#include "common.h"
#include <cstdlib>
#include <iostream>
#include <iomanip>

// 定義 cache_sim_t 的 construst，不需要回傳型態
cache_sim_t::cache_sim_t(size_t _sets, size_t _ways, size_t _linesz, const char* _name) 
: sets(_sets), ways(_ways), linesz(_linesz), name(_name), log(false)
{
  init();
}

static void help()  // 印出錯誤提示
{
  std::cerr << "Cache configurations must be of the form" << std::endl;
  std::cerr << "  sets:ways:blocksize" << std::endl;
  std::cerr << "where sets, ways, and blocksize are positive integers, with" << std::endl;
  std::cerr << "sets and blocksize both powers of two and blocksize at least 8." << std::endl;
  exit(1);
}

// cache_sim_t 的 construct 為根據 config 和 cache policy name 配置 cache
cache_sim_t* cache_sim_t::construct(const char* config, const char* name) 
{
  const char* wp = strchr(config, ':');
  if (!wp++) help();
  const char* bp = strchr(wp, ':');
  if (!bp++) help();

  size_t sets = atoi(std::string(config, wp).c_str());
  size_t ways = atoi(std::string(wp, bp).c_str());
  size_t linesz = atoi(bp);

  if (ways > 4 /* empirical */ && sets == 1)
    return new fa_cache_sim_t(ways, linesz, name);    // fully associative cache
  return new cache_sim_t(sets, ways, linesz, name);
}

void cache_sim_t::init()  // 初始化
{
  if (sets == 0 || (sets & (sets-1)))       // 若 sets==0 或是 sets 不是 2 的冪次方，則印出錯誤提示   
    help();
  if (linesz < 8 || (linesz & (linesz-1)))  // 若 block size < 8 或是 block size 不是 2 的冪次方，則印出錯誤提示
    help();

  idx_shift = 0;                            // idx_shift 為根據 linesz(block size)大小，計算 offset 所對應的 bits 數
  for (size_t x = linesz; x>1; x >>= 1)
    idx_shift++;

  tags = new uint64_t[sets*ways]();         // 'tags' 為一維陣列，存放 cache 中的有 tag values，()為初始化為0
  read_accesses = 0;
  read_misses = 0;
  bytes_read = 0;
  write_accesses = 0;
  write_misses = 0;
  bytes_written = 0;
  writebacks = 0;

  miss_handler = NULL;
}

cache_sim_t::cache_sim_t(const cache_sim_t& rhs)     
 : sets(rhs.sets), ways(rhs.ways), linesz(rhs.linesz),
   idx_shift(rhs.idx_shift), name(rhs.name), log(false)
{
  tags = new uint64_t[sets*ways];                     // 為 'tags' array 配置新記憶體空間
  memcpy(tags, rhs.tags, sets*ways*sizeof(uint64_t)); // 把 'rhs' object 的 'tags' array 內容複製給新的 'tags' array
}

cache_sim_t::~cache_sim_t()  // 'cache_sim_t' class 的 destructor, when an object of the 'cache_sim_t' class is destroyed, the destructor will be called
{
  print_stats();    
  delete [] tags;   // 釋放 'tags' array 的記憶體空間 
}

void cache_sim_t::print_stats() // 印出當前 cache 狀態資訊到螢幕上 
{
  if (read_accesses + write_accesses == 0)
    return;

  float mr = 100.0f*(read_misses+write_misses)/(read_accesses+write_accesses);  // miss rate

  std::cout << std::setprecision(3) << std::fixed;
  std::cout << name << " ";
  std::cout << "Bytes Read:            " << bytes_read << std::endl;
  std::cout << name << " ";
  std::cout << "Bytes Written:         " << bytes_written << std::endl;
  std::cout << name << " ";
  std::cout << "Read Accesses:         " << read_accesses << std::endl;
  std::cout << name << " ";
  std::cout << "Write Accesses:        " << write_accesses << std::endl;
  std::cout << name << " ";
  std::cout << "Read Misses:           " << read_misses << std::endl;
  std::cout << name << " ";
  std::cout << "Write Misses:          " << write_misses << std::endl;
  std::cout << name << " ";
  std::cout << "Writebacks:            " << writebacks << std::endl;
  std::cout << name << " ";
  std::cout << "Miss Rate:             " << mr << '%' << std::endl;
}

uint64_t* cache_sim_t::check_tag(uint64_t addr) 
{
  size_t idx = (addr >> idx_shift) & (sets-1);  // 相當於 (block address) % (sets number)，求出在哪一個 set index， (addr >> idx_shift)是在求 block address
  size_t tag = (addr >> idx_shift)  | VALID;    // 相當於 block address 並加上 VALID 位元，以產生 tag 值

  for (size_t i = 0; i < ways; i++)             // 在 selected set 中的各 ways 中尋找是否有符合的 tag 
    if (tag == (tags[idx*ways + i] & ~DIRTY))   // The & ~DIRTY operation removes the dirty bit from the retrieved tag value, which is used to indicate whether the cache block has been modified
      return &tags[idx*ways + i];               // cache hit

  return NULL;  // cache miss
}

uint64_t cache_sim_t::victimize(uint64_t addr)
{
  size_t idx = (addr >> idx_shift) & (sets-1);  // 求出需要寫入的 data address 在哪一個 set index
  size_t way = lfsr.next() % ways;              // 隨機選取其中一個 way
  uint64_t victim = tags[idx*ways + way];       // 隨機選定 selected set 中的某個 way 作為 victim block
  tags[idx*ways + way] = (addr >> idx_shift) | VALID;   // 把 data address 的 tag 寫入 victim block 原本的位置
  return victim;
}

void cache_sim_t::access(uint64_t addr, size_t bytes, bool store)
{
  store ? write_accesses++ : read_accesses++;     // increment the 'write_accesses' counter if store is true (indicating a write operation)
  (store ? bytes_written : bytes_read) += bytes;  // increment the appropriate bytes counters based on whether the access is a write or a read 

  uint64_t* hit_way = check_tag(addr);
  if (likely(hit_way != NULL))  // cache hit
  {    
    if (store)   // set DIRTY bit if cache hit and write_accesses
      *hit_way |= DIRTY;
    return;
  }

  store ? write_misses++ : read_misses++; // what kind of cache miss, increments the appropriate miss counter 
  if (log)  //  cache miss and outputs a message to the console if the `log` flag is set
  {
    std::cerr << name << " "
              << (store ? "write" : "read") << " miss 0x"
              << std::hex << addr << std::endl;
  }

  uint64_t victim = victimize(addr);  // select a victim block to be replaced, using cache replacement policy

  if ((victim & (VALID | DIRTY)) == (VALID | DIRTY))  // if the victim block is valid and dirty, write back to memory
  {
    uint64_t dirty_addr = (victim & ~(VALID | DIRTY)) << idx_shift;
    if (miss_handler)
      miss_handler->access(dirty_addr, linesz, true);
    writebacks++;
  }

  if (miss_handler)
    miss_handler->access(addr & ~(linesz-1), linesz, false);

  if (store)
    *check_tag(addr) |= DIRTY;
}

void cache_sim_t::clean_invalidate(uint64_t addr, size_t bytes, bool clean, bool inval)
{
  uint64_t start_addr = addr & ~(linesz-1);
  uint64_t end_addr = (addr + bytes + linesz-1) & ~(linesz-1);
  uint64_t cur_addr = start_addr;
  while (cur_addr < end_addr) {
    uint64_t* hit_way = check_tag(cur_addr);
    if (likely(hit_way != NULL))
    {
      if (clean) {
        if (*hit_way & DIRTY) {
          writebacks++;
          *hit_way &= ~DIRTY;
        }
      }

      if (inval)
        *hit_way &= ~VALID;
    }
    cur_addr += linesz;
  }
  if (miss_handler)
    miss_handler->clean_invalidate(addr, bytes, clean, inval);
}

// fully associative cache
fa_cache_sim_t::fa_cache_sim_t(size_t ways, size_t linesz, const char* name)  
  : cache_sim_t(1, ways, linesz, name)
{
}

uint64_t* fa_cache_sim_t::check_tag(uint64_t addr)
{
  auto it = tags.find(addr >> idx_shift);
  return it == tags.end() ? NULL : &it->second;
}

uint64_t fa_cache_sim_t::victimize(uint64_t addr)
{
  uint64_t old_tag = 0;
  if (tags.size() == ways)
  {
    auto it = tags.begin();
    std::advance(it, lfsr.next() % ways);
    old_tag = it->second;
    tags.erase(it);
  }
  tags[addr >> idx_shift] = (addr >> idx_shift) | VALID;
  return old_tag;
}
