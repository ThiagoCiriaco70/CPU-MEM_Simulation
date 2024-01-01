Files:
	main.cpp 	- main method file
	processor.h - processor code
	memory.h 	- memory code
	sample5.txt - sample that prints first 30 numbers of the fibonacci sequence

To compile and run (UNIX ONLY, C++11):

    1) Navigate to the project file

    2) Compile:

            g++ processor.h memory.h main.cpp

        OR
    
            g++ -w processor.h memory.h main.cpp 

            (to disable annoying warning bug messages)


    3) Run:
            ./a.out [text file name] [interrupt interval] 

         example:
        
            ./a.out sample5.txt 30

NOTE: compiling main.cpp may give some warnings due to a known bug with an incorrect -Wstringop-overread warning.
    
Problems or questions? Please email tac200002@utdallas.edu.
