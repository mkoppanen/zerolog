all:
	g++ -Werror -pedantic -Wall -O0 -g -Isrc src/main.cpp src/pipe.cpp -lzmq -lboost_filesystem -o zerolog
