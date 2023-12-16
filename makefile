.PHONY: all

all: main.run

main.run: main.cpp imgui/libImGUI.so shaderloader/libGLShaderLoader.so
	g++ main.cpp -g -Wl,-rpath=imgui,-rpath=shaderloader -Limgui -Lshaderloader -lGL -lGLShaderLoader -lglfw -lGLEW -lImGUI -o main.run

imgui/libImGUI.so:
	$(MAKE) -C imgui

shaderloader/libGLShaderLoader.so:
	$(MAKE) -C shaderloader

clean:
	rm -f shaderloader/libGLShaderLoader.so
	rm -f imgui/libImGUI.so
	rm -f main.run
