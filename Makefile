mpixx = $(shell mpic++ --showme)

p5: p5.cpp cluster.cpp cluster.h Makefile
	$(mpixx) p5.cpp cluster.cpp -o p5 -lpthread -Wall -Werror --std=c++20 -g

