platform = linux/x64

build   :
	g++ -c -std=c++11 -g -Wall -O1 -pipe -fPIC -I../inc/ -o test.o test.cpp
	g++ -std=c++11 -g -Wall -O1 -pipe -fPIC -o ./bin/$(platform)/cm256_codec_test test.o -L../lib/$(platform) -lcm256_codec

clean   :
	rm -rf ./bin/$(platform)/*

rebuild : clean build
