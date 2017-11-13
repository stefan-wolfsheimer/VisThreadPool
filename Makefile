CXX=g++
CXX_FLAGS=-g -std=c++11 -Wall -Isrc -I3rdparty
CXX_LIBS=-pthread

OBJ=		src/thread_pool.o\
		src/task.o
OBJ_TEST=	test/run_tests.o\
		test/test_thread_pool.o\
		test/test_task.o

OBJ_ALL=${OBJ} ${OBJ_TEST}
DEP= $(OBJ_ALL:.o=.d)

BIN_TEST=run_tests
BIN=

${BIN_TEST}: ${OBJ} ${OBJ_TEST}
	${CXX} ${CXX_FLAGS} ${CXX_LIBS} -o ${BIN_TEST} ${OBJ} ${OBJ_TEST}

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

