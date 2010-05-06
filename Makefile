APP = spacecastle

OBJECTS = main.o game.o canvas.o path.o path-parser.o spacecastle.o
HEADERS = canvas.h 

CXXFLAGS  = -g -Wall

CXXFLAGS += `pkg-config gtk+-2.0 --cflags`
LDFLAGS  += `pkg-config gtk+-2.0 --libs`

all: $(APP)

$(OBJECTS): $(HEADERS)
$(APP): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	$(RM) *.o $(APPS)
