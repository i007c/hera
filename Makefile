
CC = gcc
CFLAGS  = -std=c11 -O0 -ggdb -pedantic -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Isrc/include/ -D_GNU_SOURCE

LDFLAGS = -lX11 -lXft -lstarlight -lvulkan
# -lGL -lGLU -lGLEW -lglfw -lm
# -ldl -lXrandr -lXi -lX11 -lpthread
# -lglut 

STARLIGHT_DST = src/include/starlight.h
STARLIGHT_SRC = ../starlight/starlight.h

SOURCES  = $(shell find src -type f -name "*.c" -not -path "*/.*")
HEADERS  = $(shell find src -type f -name "*.h" -not -path "*/.*")
HEADERS += $(STARLIGHT_DST)
OBJECTS  = $(addprefix build/, $(SOURCES:.c=.o))
EXEC = bin/hera

$(EXEC): shaders clear $(OBJECTS)
	mkdir -p $(@D)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)


build/%.o: %.c $(HEADERS)
	@mkdir -p $(@D)
	@$(CC) -c $(CFLAGS) $< -o $@
	@echo $<


$(STARLIGHT_DST): $(STARLIGHT_SRC)
	cp -f $(STARLIGHT_SRC) $(STARLIGHT_DST)


run: clear $(EXEC)
	# $(EXEC) ./sample/png/y3.leaves.png
	# $(EXEC) ./sample/png/nn.png
	$(EXEC) ./sample/png/0100.png


clean:
	rm -rf $(EXEC) $(OBJECTS)


clear:
	printf "\E[H\E[3J"
	clear


install: $(EXEC)
	cp -f $< ~/.local/bin/


shaders:
	mkdir -p shader/spv/
	glslang -V shader/point.comp.glsl -o shader/spv/point.comp.spv
	glslang -V shader/cube.comp.glsl -o shader/spv/cube.comp.spv


.PHONY: clear run clean shaders
.SILENT: clear run clean $(EXEC) shaders

