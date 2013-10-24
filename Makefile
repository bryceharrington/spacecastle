APP = spacecastle

OBJECTS = main.o game.o spacecastle.o game-math.o \
	drawing.o canvas.o path.o path-parser.o 
HEADERS = game-math.h game-object.h game.h \
	drawing.h canvas.h path.h point.h

CXXFLAGS  = -g -Wall

CXXFLAGS += `pkg-config gtk+-2.0 --cflags`
LDFLAGS  += `pkg-config gtk+-2.0 --libs`

all: $(APP)

$(OBJECTS): $(HEADERS)
$(APP): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) *.o $(APPS)
