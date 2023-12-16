SRC=$(wildcard *.cpp)
OBJ=$(patsubst %.cpp, bin/%.o, $(SRC))
CPP_FLAGS=-fPIC

.phony: all

all: libGLShaderLoader.so

bin:
	mkdir bin

libGLShaderLoader.so: bin $(OBJ)
	g++ $(OBJ) --shared -o libGLShaderLoader.so -lGL

bin/%.o: %.cpp
	g++ -c $< -o $@ $(CPP_FLAGS)

clean:
	rm -f $(OBJ)