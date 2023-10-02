CXX_FLAGS=-std=c++20 -O3 -Wall -Wextra -Wpedantic 

.PHONY: all clean

all: main.out 

main.out: main.cpp
	$(CXX) $(CXX_FLAGS) main.cpp -o main.out

clean:
	$(RM) main.out
	
