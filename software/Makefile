###############################################################################
# Example makefile for openGL projects.
# ihsan@kehribar.me wrote this file.
###############################################################################
ifeq ($(shell uname), Darwin)
	LIBS = -framework GLUT -framework OpenGL
	CFLAGS += -Wno-deprecated -Wall
else	
	LIBS += -lGL -lglut -lGLU
	CFLAGS += -Wno-deprecated -Wall
endif

LIBS += -lsvm

TARGET = main

.PHONY: clean

$(TARGET):
	g++ $(TARGET).c platform.c serial_lib.c -o $(TARGET) $(CFLAGS) $(LIBS)

clean:
	rm -f *.o *~ ./$(TARGET) 

iterate:
	make clean && make 
