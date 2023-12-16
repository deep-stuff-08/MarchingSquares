SRC=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp, bin/%.o, $(SRC))
CPP_FLAGS=-fPIC

.phony: all

all: libImGUI.so

bin:
	mkdir bin

libImGUI.so: bin $(OBJ)
	g++ $(OBJ) --shared -o libImGUI.so -lglfw -lGL

bin/%.o: %.cpp
	g++ -c $< -o $@ $(CPP_FLAGS)

clean:
	rm -f $(OBJ)