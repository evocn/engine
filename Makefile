
SRC = $(wildcard src/*.cpp src/*.c) $(wildcard ./ext/imgui/*.cpp ./ext/imgui/backends/*.cpp)
CSRC = ./ext/glad/src/glad.c 
OBJ = ${SRC:.cpp=.o} ${CSRC:.c=.o}

LDFLAGS = -L. -L/opt/homebrew/Cellar/glfw/3.3.8/lib/ -L/opt/homebrew/Cellar/glm/0.9.9.8/lib -L/opt/homebrew/Cellar/freetype/2.12.1/lib/ -L/opt/homebrew/Cellar/assimp/5.2.5/lib -lglfw -lfreetype -lassimp
INCFLAGS = -I. -I./ext/glad/include -I/opt/homebrew/Cellar/glfw/3.3.8/include -I/opt/homebrew/Cellar/glm/0.9.9.8/include -I/opt/homebrew/Cellar/freetype/2.12.1/include/freetype2 -I./ext -I/opt/homebrew/Cellar/assimp/5.2.5/include -I./ext/imgui/backends/ -I./ext/imgui/
FLAGS = -std=c++11 -Wno-deprecated
OUT = engine

CC = clang++

# For debugging
print-%  : ; @echo $* = $($*)

$(OUT): $(OBJ)
	@$(CC) $(FLAGS) $(INCFLAGS) $(LDFLAGS) $^ -o $@
	@printf "\e[33mLinking\e[90m %s\e[0m\n" $@
	@printf "\e[34mDone!\e[0m\n"

%.o: %.cpp
	@$(CC) $(FLAGS) $(INCFLAGS) -c $< -o $@
	@printf "\e[36mCompile\e[90m %s\e[0m\n" $@

%.o: %.c
	@$(CC) $(FLAGS) $(INCFLAGS) -c $< -o $@
	@printf "\e[36mCompile\e[90m %s\e[0m\n" $@

./src/main.o: ./src/*.h

clean:
	@rm -f $(OUT) $(OBJ)
	@printf "\e[34mAll clear!\e[0m\n"
