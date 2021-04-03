#include <iostream>
#include <fstream>
#include <vector>
#include <string>
// #include <direct.h>
#include <string.h>
#include "cache.h"
using namespace std;


vector<Access> parse_file(int file_number){
    string file_name[4] = {
            "../test_trace_/1.trace",
            "../test_trace_/2.trace",
            "../test_trace_/3.trace",
            "../test_trace_/4.trace"
    };
    vector<Access> content;
    ifstream inFile(file_name[file_number]);
    // ifstream inFile("../test_trace/2.trace");
    if(!inFile){
        printf("error opening file\n");
        return content;
    } else{
        char line[100];
        while(inFile.getline(line, 100)){
            u64 addr = 0;
            for(int i = 2; line[i] == '0' || line[i] == '1'; i++)
                addr = (addr<<1) + (line[i]-'0');
            content.push_back(Access{addr, line[strlen(line)-1] == 'r'});
        }
    }
    return content;
}

int main() {
    vector<vector<Access>> inputs(4); // 输入文件
    for(int i = 0; i < 4; i++)
        inputs[i] = parse_file(i);
    // Cache cache(true, true, true, 8, 8);
    // cache.run(inputs[3]);
    for(int block_size: {8, 32, 64}){
        for(int n_way:{1, 4, 8, 0}){
            Cache cache(false, true, true, block_size, n_way);
            for(int i = 0; i < inputs.size(); i++){
                printf("block_size:%d, n_way:%d, input:%d\n", block_size, n_way, i);
                cache.run(inputs[i]);
            }
        }
    }
    for(bool strategy:{true, false}){
        Cache cache(strategy, true, true, 8, 8);
        for(int i = 0; i < inputs.size(); i++){
            printf("input:%d, strategy_is_tree:%d\n", i, strategy);
            cache.run(inputs[i]);
        }
    }
    for(bool back:{true, false}){
        for(bool allocation:{true, false}){
            Cache cache(true, back, allocation, 8, 8);
            for(int i = 0; i < inputs.size(); i++){
                printf("input:%d, isBack:%d, isAllocation:%d\n", i, back, allocation);
                cache.run(inputs[i]);
            }
        }
    }
    return 0;
}
