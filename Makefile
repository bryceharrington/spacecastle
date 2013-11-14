APP = spacecastle

OBJECTS = src/main.o src/game.o src/spacecastle.o src/game-math.o \
	src/drawing.o src/canvas.o src/path.o src/path-parser.o 
HEADERS = src/game-math.h src/game-object.h src/game.h \
	src/drawing.h src/canvas.h src/path.h src/point.h

CXXFLAGS  = -g -Wall

CXXFLAGS += `pkg-config gtk+-2.0 --cflags`
LDFLAGS  += `pkg-config gtk+-2.0 --libs`

all: $(APP)

$(OBJECTS): $(HEADERS)
$(APP): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o $(APPS)
