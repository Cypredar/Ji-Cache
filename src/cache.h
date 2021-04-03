#ifndef CACHE_CACHE_H
#define CACHE_CACHE_H
#endif
#include <vector>
#include <map>
#include <set>
#include <stdio.h>

typedef uint8_t u8;
typedef uint64_t u64;
typedef uint16_t u16;

struct Access{
    u64 addr;
    bool read;
};
struct Set{
    std::vector<u64> lines; // each line: || dirty | valid | 0..00 | tag ||
    // std::set<u64> tag_set; // all tag in this set
    int num = 0; // number of lines
    // u64 rep = 0; // bitmap of replace strategy
};

class Cache{
private:
    int total_size = 128*1024;
    int block_size;
    int n_set;
    int n_way;
    int len_offset;
    int len_index;
    int len_tag;
    int log_way;
    bool tree; // binary tree
    bool write_back; // write back
    bool write_allocation; // write allocation
    u16 *rep; // replace vector
    std::vector<Set> table; // cache itself
    // FILE *file;

public:
    Cache(bool tree, bool back, bool allocation, int block, int way);
    ~Cache() = default;

    void run(const std::vector<Access>& input);
    int read(u64 addr);
    int write(u64 addr);
    int get_idx(u64 addr) const;
    u64 get_tag_from_addr(u64 addr) const;
    u64 get_tag_from_line(u64 line) const;
    void refresh(int idx, u64 tag, int pos);
    void replace(int idx, u64 tag, int pos);
    static u64 make_line(u64 tag, u64 valid, u64 dirty);
};
