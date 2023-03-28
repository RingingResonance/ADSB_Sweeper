adsbSweeper: vector.o
	g++ -std=c++11 -pthread -o ./adsbSweeper ./adsbSweeper.o -lgpiod

vector.o: adsbSweeper.cpp
	g++ -std=c++11 -pthread -c ./adsbSweeper.cpp -o ./adsbSweeper.o -DLINUX_BUILD -lgpiod

clean:
	rm *.o
