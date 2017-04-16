CXX=g++
CXX_FLAGS=-g -std=c++11 -Wall
CXX_LIBS=-lpthread

OBJ=thread_pool.o

BIN=thread_pool

thread_pool: thread_pool.o ${OBJ}
	${CXX} ${CXX_FLAGS} ${CXX_LIBS} -o thread_pool ${OBJ}

.PONY: dep clean

dep: ${DEP}

clean:
	rm -f ${DEP}
	rm -f ${OBJ_ALL}
	rm -f ${BIN_TEST}


%.d: %.cpp
	${CXX} -MM ${CXX_FLAGS} ${INCLUDE} $*.cpp > $*.d

-include $(OBJ:.o=.d)

%.o: %.cpp
	${CXX} ${CXX_FLAGS} -c -o $@ $<

