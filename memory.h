
#include<iostream>
class Memory {

public:
    inline static int mem_size = 2000;
    inline static int memory[2000];

    Memory(){

    }

    //MEMORY METHODS

    static int writeAddress(int addr, int data){
        if(addr < 0 || addr >= mem_size){ //check if addr is OOB
            std::cerr << "Memory write failure. Memory index out of bounds: " << addr << "\n";
            exit(EXIT_FAILURE);
        }
        memory[addr] = data; //write data to mem addr
        return 0; //signal successfusl write
    }

    static int readAddress(int addr){
        if(addr < 0 || addr >= mem_size){
            std::cerr << "Memory read failure. Memory index out of bounds: " << addr << "\n";
            exit(EXIT_FAILURE);
        }
        return memory[addr];
    }
    
};