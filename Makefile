CC=gcc
CFLAGS=-g -DMG_ENABLE_CALLBACK_USERDATA=1 -Wall
DEPFLAGS= -MT $@ -MMD -MP -MF $*.td
CXX=g++
CXX_FLAGS=-g -std=c++11 -Wall -Isrc -I3rdparty -I3rdparty/Catch2/single_include
CXX_LIBS=-pthread

COBJ=           3rdparty/mongoose/mongoose.o
OBJ=		src/thread_pool.o\
		src/task.o\
		src/server.o \
		src/n_queens.o
OBJ_BIN=	src/server_main.o
OBJ_TEST=	test/run_tests.o\
		test/test_thread_pool.o\
		test/test_task.o\
		test/test_n_queens.o

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
	rm -f ${COBJ}

%.o: %.cpp
	${CXX} ${CXX_FLAGS} ${DEPFLAGS} -c -o $@ $<
	mv $*.td $*.d 
.d: ;
.PRECIOUS: %.d

src/server.o: src/server.cpp src/index.inc
src/index.inc: src/index.html
	( echo 'index = R"__HTML__(' && \
	  cat src/index.html && \
	  echo ')__HTML__";' ) > src/index.inc

-include $(OBJ_ALL:.o=.d)


