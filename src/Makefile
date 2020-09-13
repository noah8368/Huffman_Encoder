objects = encoder.o main.o
exe_name = huffman_encoder

$(exe_name) : $(objects)
	g++ -o $(exe_name) $(objects)

encoder.o : encoder.h constants.h
	g++ -std=c++11 -c encoder.cpp 
main.o : encoder.h constants.h
	g++ -std=c++11 -c main.cpp
.PHONY : clean
clean :
	-rm $(exe_name) $(objects)
