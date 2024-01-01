

#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include "memory.h"
#include "processor.h"
using namespace std;

// CONSTANTS
const int MSG_SIZE = 10; // 9 chars + null terminator
//NOTE: this msg size used to be 4, but was made larger for my fibonacci sequence program

// global values
int timerInterval;

int main(int argc, char *argv[])
{

    // validate arguments
    if (argc != 3)
    {
        cerr << "Incorrect format. Format should be: ./main [inputfilename] [cpu timer interval]\n";
        exit(EXIT_FAILURE);
    }

    ifstream testfile(argv[1]);
    if (!testfile)
    {
        cerr << "File not found : " << argv[1] << "\n";
        exit(EXIT_FAILURE);
    }
    testfile.close();

    int i = 0;
    while (argv[2][i] != '\0')
    {
        if (!isdigit(argv[2][i]))
        {
            cerr << "Format error. Timer interval must be an integer.\n";
            exit(EXIT_FAILURE);
        }
        i++;
    }
    timerInterval = atoi(argv[2]);

    // pipe setup
    int pid;
    int ptm_pipe[2]; // pipe for processor -> memory requests
    int mtp_pipe[2]; // pipe for memory -> processor data

    // pipe creation
    if (pipe(ptm_pipe) < 0 || pipe(mtp_pipe) < 0)
    {
        cerr << "Pipe failure.\n";
        exit(EXIT_FAILURE);
    }

    // fork process
    pid = fork();
    if (pid < 0)
    {
        cerr << "Fork failure.\n";
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    { // child processes

        // MEMORY CODE
        Memory mem = Memory(); // main memory object

        // INITAL FILE READ
        ifstream progf(argv[1]); // open program file stream

        string s = "";
        int mindex = 0;

        while (getline(progf, s))
        {
            string value = "";
            int i = 0;
            bool jump = false;

            if (s == "" || s == "\n" || s == " ")
            { // ignore empty lines
                continue;
                cout << "empty!" << flush;
            }

            if (s[i] == '.')
            { // period indicating a jump
                jump = true;
                i++;
            }

            while (i < s.length() && isdigit(s[i]))
            { // grab numerical portion
                value += s[i];
                i++;
            }

            if (jump) // handle jumping to an address to write (period in input file)
            {
                mindex = stoi(value);
            }
            else if (value.length() > 0)
            {
                mem.writeAddress(mindex, stoi(value));
                mindex++;
            }
            s = "";
        }

        // MAIN LOOP
        int nbytes;
        char buffer[MSG_SIZE];
        while ((nbytes = read(ptm_pipe[0], buffer, MSG_SIZE)) > 0)
        {
            string action = buffer;

            if (action[0] == 'e') //case that CPU signalled end
            {
                exit(0);
            }

            read(ptm_pipe[0], buffer, MSG_SIZE);
            string addr_str = buffer;

            // read request
            if (action[0] == 'r') //CPU signalled read
            {
                int reply = mem.readAddress(stoi(addr_str));
                write(mtp_pipe[1], to_string(reply).c_str(), MSG_SIZE); // read
            }
            else if (action[0] == 'w') // CPU signalled write
            {
                read(ptm_pipe[0], buffer, MSG_SIZE); //what to write
                string data_str = buffer;
                mem.writeAddress(stoi(addr_str), stoi(data_str));
                write(mtp_pipe[1], "0", MSG_SIZE); // signal successful write
            }
        }
        if (nbytes == -1) //case of read failure
        {
            cerr << "Memory: pipe read failure.\n";
            exit(EXIT_FAILURE);
        }
        exit(0);
    }

    //PARENT - PROCESSOR CODE
    Processor cpu = Processor(ptm_pipe, mtp_pipe, timerInterval);

    while (true)
    { // execute call will terminate on end instruction
        cpu.fetch();
        cpu.execute();
    }
    exit(0);
}
