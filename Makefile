all:
	g++ -std=c++11 -o server server.cpp -lpthread

clean:
	rm server
