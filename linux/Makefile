TARGET = reacTIVision
CSOURCES = $(shell ls ../ext/libfidtrack/*.c ../ext/portvideo/interface/*.c)  
COBJECTS = $(CSOURCES:.c=.o)
CPPSOURCES = $(shell ls ../common/*.cpp ../ext/libfidtrack/*.cpp ../ext/portvideo/linux/*.cpp ../ext/portvideo/common/*.cpp ../ext/portvideo/interface/*.cpp ../ext/portvideo/tinyxml/*.cpp ) 
CPPOBJECTS = $(CPPSOURCES:.cpp=.o)
OSCSOURCES = $(shell ls ../ext/oscpack/ip/posix/*.cpp ../ext/oscpack/osc/*.cpp) 
OSCOBJECTS = $(OSCSOURCES:.cpp=.o)

SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

INCLUDES = -I../ext/oscpack -I../ext/libfidtrack -I../ext/portvideo/linux -I../ext/portvideo/common -I../ext/portvideo/interface -I../ext/portvideo/tinyxml -I../common
COPTS  = -Ofast -w
CDEBUG = -Wall -g
CFLAGS = $(COPTS) $(SDL_CFLAGS) $(INCLUDES) -DLINUX -DOSC_HOST_LITTLE_ENDIAN -DNDEBUG
#CFLAGS = $(CDEBUG) $(SDL_CFLAGS) $(INCLUDES) -DLINUX -DOSC_HOST_LITTLE_ENDIAN
CXXFLAGS = $(CFLAGS)

LIBS = $(SDL_LDFLAGS) -ldc1394 -lpthread -lturbojpeg

all:	$(TARGET) 

$(TARGET) : $(CPPOBJECTS) $(COBJECTS) $(OSCOBJECTS) $(PMOBJECTS)
	$(CXX) -o $@ $+ $(LIBS) 
#	strip $(TARGET)

clean:
	rm -f $(TARGET) $(CPPOBJECTS) $(COBJECTS) $(OSCOBJECTS) $(PMOBJECTS) core*

run:	$(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 755 -c $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 644 -c ../README.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 644 -c ../LICENSE.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 644 -c ../CHANGELOG.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 644 -c ../GPL.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 666 -c *.xml $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -m 666 -c *.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 666 -c *.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 666 -c *.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 666 -c *.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -d $(DESTDIR)$(PREFIX)/share/$(TARGET)/amoeba
	install -m 666 -c ../symbols/amoeba/*.pdf $(DESTDIR)$(PREFIX)/share/$(TARGET)/amoeba
	install -m 666 -c ../symbols/amoeba/*.trees $(DESTDIR)$(PREFIX)/share/$(TARGET)/amoeba
	install -d $(DESTDIR)$(PREFIX)/share/$(TARGET)/classic
	install -m 666 -c ../symbols/classic/*.png $(DESTDIR)$(PREFIX)/share/$(TARGET)/classic
	install -d $(DESTDIR)$(PREFIX)/share/$(TARGET)/calibration
	install -m 666 -c ../calibration/*.pdf $(DESTDIR)$(PREFIX)/share/$(TARGET)/calibration
	install -d $(DESTDIR)$(PREFIX)/share/$(TARGET)/midi
	install -m 666 -c ../midi/* $(DESTDIR)$(PREFIX)/share/$(TARGET)/midi

