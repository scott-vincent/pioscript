CC = g++
DEBUG = -g
NODEBUG = -O
EXEDIR = /usr/local/bin
EXE = $(EXEDIR)/pioscript
SRC = $(wildcard *.cpp)
OBJDIR = obj
OBJS = $(SRC:%.cpp=$(OBJDIR)/%.o)

SDL_CFLAGS = $(shell sdl-config --cflags)
SDL_LFLAGS = $(shell sdl-config --libs)

OTHER_CFLAGS = -I/usr/local/include
OTHER_LFLAGS = -L/usr/local/lib -lwiringPi -lSDL_mixer -lportaudio

CFLAGS = $(NODEBUG) $(SDL_CFLAGS) $(OTHER_CFLAGS) -std=c++0x -pthread
LFLAGS = $(SDL_LFLAGS) $(OTHER_LFLAGS)

.PHONY: all
all: $(EXE)

$(EXE) : $(OBJDIR) $(OBJS)
	@echo "------------------------"
	@echo "  Linking "$@
	@echo "------------------------"
	$(CC) -o $@ $(OBJS) $(CFLAGS) $(LFLAGS)
	@chmod 4755 /usr/local/bin/pioscript

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/main.o: main.cpp
	@echo "------------------------"
	@echo "  Compiling "$<
	@echo "------------------------"
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(OBJDIR)/%.o: %.cpp %.h command.h
	@echo "------------------------"
	@echo "  Compiling "$<
	@echo "------------------------"
	$(CC) -c -o $@ $< $(CFLAGS)
	
.PHONY: clean
clean:
	@echo "------------------------"
	@echo "  Cleaning"
	@echo "------------------------"
	rm -f $(OBJS) $(EXE)
	rmdir $(OBJDIR)
