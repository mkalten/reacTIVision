TARGET = SportVideo
CSOURCES = $(shell ls ../interface/*.c)
COBJECTS = $(CSOURCES:.c=.o)
CPPSOURCES = $(shell ls ./*.cpp ../sdldemo/*.cpp ../common/*.cpp ../interface/*.cpp ../tinyxml/*.cpp)
CPPOBJECTS = $(CPPSOURCES:.cpp=.o)

SDL_CFLAGS  := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)

INCLUDES = -I../common -I../sdldemo -I../interface -I../tinyxml -I.
CFLAGS_REL = -Ofast -w $(SDL_CFLAGS) $(INCLUDES) -DLINUX -DNDEBUG
CFLAGS_DBG = -Wall -g $(SDL_CFLAGS) $(INCLUDES) -DLINUX -DDEBUG
all: CFLAGS = $(CFLAGS_REL)
all: CXXFLAGS = $(CFLAGS_REL)
debug: CFLAGS = $(CFLAGS_DBG)
debug: CXXFLAGS = $(CFLAGS_DBG)

LIBS = $(SDL_LDFLAGS) -ldc1394 -lpthread -lturbojpeg

all: $(TARGET)

debug: $(TARGET)

%.o: %.cpp
	@echo [CXX] $@
	@ $(CXX) $(CFLAGS) -o $@ -c $<
%.o: %.c
	@echo [CC] $@
	@ $(CC) $(CFLAGS) -o $@ -c $<


$(TARGET) : $(CPPOBJECTS) $(COBJECTS)
	@echo [LD] $@
	@ $(CXX) -o $@ $+ $(LIBS)

clean:
	@echo [CLEAN] $(TARGET)
	@ rm -f $(TARGET) $(CPPOBJECTS) $(COBJECTS) core*

run:	$(TARGET)
	./$(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 -c $(TARGET) $(DESTDIR)$(BINDIR)
	install -m 644 -c ../readme.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 644 -c ../license.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -m 644 -c ../changelog.txt $(DESTDIR)$(PREFIX)/share/$(TARGET)
	install -d $(DESTDIR)$(PREFIX)/share/applications/
	install -m 666 -c *.desktop $(DESTDIR)$(PREFIX)/share/applications/
	install -d $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -m 666 -c *.png $(DESTDIR)$(PREFIX)/share/pixmaps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -m 666 -c *.png $(DESTDIR)$(PREFIX)/share/icons/hicolor/128x128/apps/
	install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
	install -m 666 -c *.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/apps/
