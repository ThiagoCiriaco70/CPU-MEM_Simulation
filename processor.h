
#include<iostream>
#include<unistd.h>
#include<string>
#include<cstring>
#include<stdlib.h>
class Processor {
private:
    int PC;
    int SP;
    int IR;
    int AC;
    int X;
    int Y;
    bool kernel_mode; //true for kernel mode, false for user mode
    int timer;
    int interval;
    int ptm_p[2];
    int mtp_p[2];
    static const int MSG_SIZE = 10;
    static const int USER_STACK_START = 999;
    static const int SYS_STACK_START = 1999;
    char buffer[5];
    int nbytes;

    //CHANGE FOR DEBUG
    static const bool DEBUG = false;

    
public:
    
    Processor(int ptm_pipe[2], int mtp_pipe[2], int int_timer){
        ptm_p[1] = ptm_pipe[1];
        ptm_p[0] = ptm_pipe[0];
        mtp_p[0] = mtp_pipe[0];
        mtp_p[1] = mtp_pipe[1];
        interval = int_timer;
        timer = 0;
        nbytes = 0;
        PC = 0;
        SP = USER_STACK_START;
        IR = 0;
        AC = 0;
        X = 0;
        Y = 0;
        kernel_mode = false;;
        

        //INITAL READ OF ADDRESS 0 ON CREATION

        const char* pcAsChars = std::to_string(PC).c_str();

        write(ptm_pipe[1], "r", MSG_SIZE); //read
        write(ptm_pipe[1], pcAsChars, MSG_SIZE); //PC

        //read first instruction
        nbytes = read(mtp_pipe[0], buffer, MSG_SIZE);

        if(DEBUG){std::cout << "Processor communicating with memory\n";}

        IR = std::stoi(std::string(buffer));
    }

    //PROCESSOR HELPER METHODS
    int fetch(){

        //get next instruction
        IR = getMemAt(PC);

        if((timer%interval == 0) && (timer > 0) && !kernel_mode){
            interrupt(0); //timer interrupt
        }

        return 0;
    }

    int getNextMem(){
        int next = getMemAt(PC);
        PC++;
        return next;
    }

    int getMemAt(int addr){

        if(addr > USER_STACK_START && !kernel_mode){ //check for permissions
            std::cerr << "Memory violation: read memory at address " << addr << " in user mode.\n";
        }

        const char* addrStr = std::to_string(addr).c_str();
        write(ptm_p[1], "r", MSG_SIZE); //read
        write(ptm_p[1], addrStr, MSG_SIZE); //PC

        //read next
        nbytes = read(mtp_p[0], buffer, MSG_SIZE);
        if(DEBUG){std::cout << "Read address " << addr << ": " << buffer << "\n";}
        return std::stoi(std::string(buffer));
    }

    int writeMemAt(int addr, int data){

        if(addr > USER_STACK_START && !kernel_mode){ //check for permissions
            std::cerr << "Memory violation: write memory at address " << addr << " in user mode.\n";
            end();
        }

        const char* pcAsChars = std::to_string(addr).c_str();
        write(ptm_p[1], "w", MSG_SIZE); //write
        //std::cout << "Processor write: " << "w" << "\n";
        write(ptm_p[1], (std::to_string(addr).c_str()), MSG_SIZE); //write data
        //std::cout << "Processor write: " << addr << "\n";
        write(ptm_p[1], (std::to_string(data).c_str()), MSG_SIZE); //write data
        //std::cout << "Processor write: " << data << "\n";

        if(DEBUG){std::cout << "Write to addr " << addr << ": " << data << "\n";}

        //read 0 back to confirm write
        nbytes = read(mtp_p[0], buffer, MSG_SIZE);

        if(DEBUG){std::cout << "Processor confirmed successful write" << "\n";}

        return std::stoi(std::string(buffer));
    }

    int execute(){
        PC++;
        switch(IR){
            case 1:
                loadVal(getNextMem());
                break; 
            case 2:
                loadValAddr(getNextMem());
                break; 
            case 3:
                loadInd(getNextMem());
                break; 
            case 4:
                loadIndX(getNextMem());
                break; 
            case 5:
                loadIndY(getNextMem());
                break; 
            case 6:
                loadSpX();
                break; 
            case 7:
                store(getNextMem());
                break; 
            case 8:
                get();
                break; 
            case 9:
                put(getNextMem());
                break; 
            case 10:
                addX();
                break; 
            case 11:
                addY();
                break; 
            case 12:
                subX();
                break; 
            case 13:
                subY();
                break; 
            case 14:
                copyToX();
                break; 
            case 15:
                copyFromX();
                break; 
            case 16:
                copyToY();
                break; 
            case 17:
                copyFromY();
                break; 
            case 18:
                copyToSp();
                break; 
            case 19:
                copyFromSp();
                break; 
            case 20:
                jump(getNextMem());
                break; 
            case 21:
                jumpIfEq(getNextMem());
                break; 
            case 22:
                jumpIfNeq(getNextMem());
                break; 
            case 23:
                call(getNextMem());
                break; 
            case 24:
                ret();
                break; 
            case 25:
                incX();
                break; 
            case 26:
                decX();
                break; 
            case 27:
                push();
                break; 
            case 28:
                pop();
                break; 
            case 29:
                interrupt(1); //instruction interrupt
                break; 
            case 30:
                intRet();
                break;            
            case 50:
                end();
                break;
            default:
                std::cerr << "Bad instruction in execute phase: " << IR << "\n"; 
                end();
                break;
        }
        timer++; //increment timer
        return 0;
    }

    //PROCESSOR FUCNTION METHODS

    int getPC(){
        return PC;
    }

    int setIR(int next){
        IR = next;
        return 0;
    }

    int getIR(){
        return IR;
    }

    //1 Load the value into the AC
    int loadVal(int val){
        AC = val;
        return 0;
    }

    //2 Load the value at the address into the AC
    int loadValAddr(int addr){
        AC = getMemAt(addr);
        return 0;
    }

    //3 Load the value from the address found in the given address into the AC
    //(for example, if LoadInd 500, and 500 contains 100, then load from 100).
    int loadInd(int addr){
        int addr2 = getMemAt(addr);
        AC = getMemAt(addr2);
        return 0;
    }

    //4 Load the value at (address+X) into the AC
    //(for example, if LoadIdxX 500, and X contains 10, then load from 510).
    int loadIndX(int addr){
        AC = getMemAt(X + addr);
        return 0;
    }

    //5 Load the value at (address+Y) into the AC
    int loadIndY(int addr){
        AC = getMemAt(Y + addr);
        return 0;
    }
    
    //6 Load from (Sp+X) into the AC (if SP is 990, and X is 1, load from 991).
    int loadSpX(){
        AC = getMemAt(SP + X);
        return 0;
    }

    //7 Store the value in the AC into the address
    int store(int addr){
        writeMemAt(addr, AC);
        return 0;
    }

    //8 Gets a random int from 1 to 100 into the AC
    int get(){
        srand(time(0));
        AC = rand()%100 + 1;
        return 0;
    }

    //9 If port=1, writes AC as an int to the screen
    //If port=2, writes AC as a char to the screen
    int put(int port){
        if(port == 1){
            std::cout << AC << std::flush;
            return 0;
        } else if(port == 2){
            char c = AC;
            std::cout << c << std::flush;
            return 0;
        }
        return -1;
    }

    //10 Add the value in X to the AC
    int addX(){
        AC = AC + X;
        return 0;
    }

    //11 Add the value in Y to the AC
    int addY(){
        AC = AC + Y;
        return 0;
    }

    //12 Subtract the value in X from the AC
    int subX(){
        AC = AC - X;
        return 0;
    }

    //13 Subtract the value in Y from the AC
    int subY(){
        AC = AC - Y;
        return 0;
    }

    //14 Copy the value in the AC to X
    int copyToX(){
        X = AC;
        return 0;
    }

    //15 Copy the value in X to the AC
    int copyFromX(){
        AC = X;
        return 0;
    }

    //16 Copy the value in the AC to Y
    int copyToY(){
        Y = AC;
        return 0;
    }

    //17 Copy the value in Y to the AC
    int copyFromY(){
        AC = Y;
        return 0;
    }

    //18 Copy the value in AC to the SP
    int copyToSp(){
        SP = AC;
        return 0;
    }

    //19 Copy the value in SP to the AC
    int copyFromSp(){
        AC = SP;
        return 0;
    }

    //20 Jump to the address
    int jump(int addr){
        PC = addr;
        return 0;
    }

    //21 Jump to the address only if the value in the AC is zero
    int jumpIfEq(int addr){
        if(AC == 0){
            PC = addr;
        }
        return 0;
    }

    //22 Jump to the address only if the value in the AC is not zero
    int jumpIfNeq(int addr){
        if(AC != 0){
            PC = addr;
        }
        return 0;
    }

    //23 Push return address onto stack, jump to the address
    int call(int addr){
        SP--;
        writeMemAt(SP, PC);
        PC = addr;

        
        return 0;
    }

    //24 Pop return address from the stack, jump to the address
    int ret(){
        PC = getMemAt(SP);
        SP++;
        return 0;
    }

    //25 Increment the value in X
    int incX(){
        X++;
        return 0;
    }

    //26 Decrement the value in X
    int decX(){
        X--;
        return 0;
    }

    //27 Push AC onto stack
    int push(){
        SP--;
        writeMemAt(SP, AC);
        return 0;
    }

    //28 Pop from stack into AC
    int pop(){
        AC = getMemAt(SP);
        SP++;
        return 0;
    }

    //29 Perform system call.
    // Type 0 indicates timer interrupt
    // Type 1 indicates an int instruction
    int interrupt(int type){

        kernel_mode = true;
        int tempSP = SP;

        SP = SYS_STACK_START;

        //push old SP to stack
        SP--;
        writeMemAt(SP, tempSP);

        //push old PC to stack
        SP--;
        writeMemAt(SP, PC);



        if(type == 1){ //int instruction
            PC = 1500;
        } else if (type == 0){ //timer
            PC = 1000; //becomes 1000 at next fetch
            IR = getMemAt(PC);
        } else {
            return -1;
        }
        return 0;
    }

    //30 Return from system call
    int intRet(){

        //pop old PC back
        PC = getMemAt(SP);
        SP++;

        //pop old SP back
        int tempSP = getMemAt(SP);
        SP++;
        SP = tempSP;

        kernel_mode = false;
        return 0;
    }

    //50 End execution
    void end(){
        write(ptm_p[1], "end", MSG_SIZE); //tell memory to end its process
        usleep(100000); //give memory time to read
        close(ptm_p[1]); //close ptm fds
        close(ptm_p[0]); 
        close(mtp_p[1]); //close mtp fds
        close(mtp_p[0]); 

        exit(0); //end processor process
    }
};