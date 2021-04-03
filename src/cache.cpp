#include "cache.h"
#include <iostream>
#include <cmath>

using namespace std;

int debug = 0;
int rando = 0;

Cache::Cache(bool binary_tree, bool back, bool allocation, int block, int way){
    tree = binary_tree;
    write_back = back;
    write_allocation = allocation;
    block_size = block;
    if(way == 0){ // full-associative
        n_way = total_size / block_size;
        n_set = 1;
    } else{ // 1 4 8
        n_way = way;
        n_set = total_size / block_size / n_way;
    }
    // assert(block_size * n_way * n_set == total_size);
    len_offset = log2(block_size);
    len_index = log2(n_set);
    log_way = log2(n_way); // 组内秩的长度
    len_tag = 64 - len_offset - len_index;
    rep = new u16[n_set*n_way];
    for(int i = 0; i < n_set*n_way; i++) rep[i] = 0;
    Set init_set = {
            vector<u64>(n_way),
            // set<u64>(),
            0
    }; // 初始化set
    table = vector<Set>(n_set, init_set);
    // file = fopen("../4.log", "w");
    // cout<<"block_size:"<<block_size<<" n_way:"<<n_way<<" n_set:"<<n_set<<" len_off:"<<len_offset<<" len_idx:"<<len_index<<" len_tag:"<<len_tag<<endl;
    // cout<<"n_way*n_set:"<<n_way*n_set<<endl;
}

void Cache::refresh(int idx, u64 tag, int pos){ // hit
    if(debug)printf("refresh\n");
    Set &s = table[idx]; // ok
	if(rando) return;
    if(tree){
        int dst = 0;
        while(dst != n_way){
            if(get_tag_from_line(s.lines[dst]) == tag) break;
            dst++;
        } // 找到对应tag的位置
        int i = 0;
        for(int j = log_way; j > 0; j--){
            if((dst >> (j-1)) & 1){
                rep[pos+i] = 0;
                i = i*2 + 2;
            } else{
                rep[pos+i] = 1;
                i = i*2 +1;
            }
        }
    } else{ // lru
        int dst = 0;
        while(dst != n_way){
            if(get_tag_from_line(s.lines[dst]) == tag) break;
            dst++;
        } // 找到对应tag的位置
        int x;
        for(x = 0; x < s.num; x++)
            if(rep[pos+x] == dst) break;
        for(int i = x; i > 0; i--)
            rep[pos+i] = rep[pos+i-1];
        rep[pos] = dst;
    }
}


void Cache::replace(int idx, u64 tag, int pos){ // not hit
    if(debug)printf("replace\n");
    Set &s = table[idx]; // ok
    u64 new_line = make_line(tag, 1, 0);
    if(rando){
        int r = rand() % n_way;
        s.lines[r] = new_line;
        return;
    }
	if(tree){
        int i = 0;
        for(int j = 0; j < log_way; j++){
            if(rep[pos+i]){
                rep[pos+i] = 0;
                i = i*2 + 2;
            } else {
                rep[pos+i] = 1;
                i = i*2 + 1;
            }
        }
        int dst = i + 1 - n_way;
        s.lines[dst] = new_line;
    } else{
        if(s.num < n_way){
            s.lines[s.num] = new_line;
            //s.tag_set.insert(tag);
            for(int i = s.num ; i > 0; i--)
                rep[pos+i] = rep[pos+i-1];
            rep[pos] = s.num;
        } else{
            int x = rep[pos+n_way-1];
            s.lines[x] = new_line;
            for(int i = n_way-1; i > 0; i--)
                rep[pos+i] = rep[pos+i-1];
            rep[pos] = x;
        }
    }
    if(s.num < n_way) s.num++;
}

int Cache::read(u64 addr){
    int idx = get_idx(addr);
    u64 tag = get_tag_from_addr(addr);
    int pos = n_way * idx; // rep起始，长度为n_way
    // if(debug)printf("read--idx:%x pos:%x tag:%llx\n", idx, pos, tag);
    bool isHit = false;
    for(int i = 0; i < n_way; i++){
        u64 line = table[idx].lines[i];
        u64 tag_in_line = get_tag_from_line(line);
        if (tag_in_line == tag){
            isHit = true;
            break;
        }
    }
    // if(table[idx].tag_set.count(tag) > 0){ // hit
    if(isHit){
        // refresh, dst+pos
        refresh(idx, tag, pos);
        if(debug)printf("read-hit\n");
        // fprintf(file, "Hit\n");
        return true;
    } else{ // not hit
        // replace
        replace(idx, tag, pos);
        if(debug)printf("read-miss\n");
        // fprintf(file, "Miss\n");
        return false;
    }
}

int Cache::write(u64 addr){
    int idx = get_idx(addr);
    u64 tag = get_tag_from_addr(addr);
    int pos = n_way * idx; // rep起始，长度为n_way
    // if(debug)printf("write--idx:%x pos:%x tag:%llx\n", idx, pos, tag);
    bool isHit = false;
    for(int i = 0; i < n_way; i++){
        u64 line = table[idx].lines[i];
        u64 tag_in_line = get_tag_from_line(line);
        if (tag_in_line == tag){
            isHit = true;
            break;
        }
    }
    //if(table[idx].tag_set.count(tag) > 0){ // hit
    if(isHit){
        // refresh(idx, tag, pos);
        if(write_back){ // write back
            refresh(idx, tag, pos);
            u64 line = make_line(tag, 1, 1);
            // set dirty
        } else{ // directly write
            refresh(idx, tag, pos);
        }
        return true;
    } else{ // not hit
        if(write_allocation){ // write allocation
            replace(idx, tag, pos);
        } else{ // not allocation
            // do noting ??
        }
        return false;
    }
}

u64 Cache::make_line(u64 tag, u64 valid, u64 dirty){
    return (dirty << 63) | (valid << 62) | tag;
}

int Cache::get_idx(u64 addr) const{
    u8 idx_shift = len_offset;
    int idx_mask = (1 << len_index) - 1;
    return (int)(addr >> idx_shift) & idx_mask;
}

u64 Cache::get_tag_from_addr(u64 addr) const{
    u8 tag_shift = len_index + len_offset;
    u64 tag_mask = (1ULL << len_tag) - 1;
    return (addr >> tag_shift) & tag_mask;
}

u64 Cache::get_tag_from_line(u64 line) const{
    u64 tag_mask = (1ULL << len_tag) - 1;
    // printf("get tag%llx from line%llx\n", line&tag_mask, line);
    return line & tag_mask;
}

void Cache::run(const std::vector<Access>& input){
    int r = 0, w = 0;
    int r_hit = 0, w_hit = 0;
    for(auto i : input){
        // cout << "input: " << i.addr << " " << i.read << endl;
        if(i.read){
            r++;
            r_hit += read(i.addr);
        } else{
            w++;
            w_hit += write(i.addr);
        }
    }
    // printf("r:%d w:%d r_hit:%d w_hit:%d\n", r, w, r_hit, w_hit);
    float loss = 1 - 1.0*(r_hit+w_hit)/(r+w);
    printf("loss:%f\n", loss);
}
