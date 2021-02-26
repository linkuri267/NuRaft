OBJS = main.o
CXX = g++
# CXXFLAGS = -Wall std=c++11
LDLIBS = -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

# LDFLAGS = "-L/c/SFML-2.5.1/lib"
# CPPFLAGS = "-L/c/SFML-2.5.1/include"

game: $(OBJS)
	$(CXX) -o game $(OBJS) $(LDLIBS) $(LDFLAGS)

$(OBJS): main.cpp Bomb.cpp
	$(CXX) -c main.cpp

Bomb.c: Bomb.cpp Waves.cpp
	$(CXX) -c Bomb.cpp

# Waves.c: Waves.cpp
# 	$(CXX) -c Waves.cpp

clean: 
	$(RM) game $(OBJS)