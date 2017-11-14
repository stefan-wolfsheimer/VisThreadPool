CC=gcc
CFLAGS=-g -Wall
CXX=g++
CXX_FLAGS=-g -std=c++11 -Wall -Isrc -I3rdparty -I3rdparty/Catch2/single_include
CXX_LIBS=-pthread

COBJ=           3rdparty/mongoose/mongoose.o
OBJ=		src/thread_pool.o\
		src/task.o\
		src/server.o
OBJ_BIN=	src/server_main.o
OBJ_TEST=	test/run_tests.o\
		test/test_thread_pool.o\
		test/test_task.o

OBJ_ALL=${OBJ} ${OBJ_TEST} ${OBJ_BIN}
DEP= $(OBJ_ALL:.o=.d)

BIN_TEST=run_tests
BIN=server

all: ${BIN_TEST} ${BIN}


${BIN_TEST}: ${OBJ} ${OBJ_TEST} ${COBJ}
	${CXX} ${CXX_FLAGS} ${CXX_LIBS} -o ${BIN_TEST} ${OBJ} ${OBJ_TEST} ${COBJ}

${BIN}: ${OBJ} ${OBJ_BIN} ${COBJ}
	${CXX} ${CXX_FLAGS} ${CXX_LIBS} -o ${BIN} ${OBJ} ${COBJ} ${OBJ_BIN}

.PONY: dep clean

dep: ${DEP}

clean:
	rm -f ${DEP}
	rm -f ${OBJ_ALL}
	rm -f ${BIN_TEST}


%.d: %.cpp
	${CXX} -MM ${CXX_FLAGS} ${INCLUDE} $*.cpp > $*.d

-include $(OBJ_ALL:.o=.d)

%.o: %.cpp
	${CXX} ${CXX_FLAGS} -c -o $@ $<

