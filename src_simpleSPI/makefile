main: adsbSweeper.o ADSBdata.o CLIscope.o DACscope.o
	g++ -std=c++20 -pthread -o ../adsbSweeper ./lib/adsbSweeper.o ./lib/ADSBdata.o ./lib/CLIscope.o ./lib/DACscope.o -lgpiod

adsbSweeper.o: adsbSweeper.cpp
	g++ -std=c++20 -pthread -c ./adsbSweeper.cpp -o ./lib/adsbSweeper.o -DLINUX_BUILD -lgpiod

ADSBdata.o: ADSBdata.cpp
	g++ -std=c++20 -pthread -c ./ADSBdata.cpp -o ./lib/ADSBdata.o -DLINUX_BUILD -lgpiod

DACscope.o: DACscope.cpp
	g++ -std=c++20 -pthread -c ./DACscope.cpp -o ./lib/DACscope.o -DLINUX_BUILD -lgpiod

CLIscope.o: CLIscope.cpp
	g++ -std=c++20 -pthread -c ./CLIscope.cpp -o ./lib/CLIscope.o -DLINUX_BUILD -lgpiod

clean:
	rm ./lib/*.o
