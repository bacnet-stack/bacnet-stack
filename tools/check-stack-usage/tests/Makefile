OBJ:=$(patsubst %.c, %.o, $(wildcard *.c))
CFLAGS:=-fstack-usage
TARGET:=main
CC?=gcc

all:	${TARGET}
	@echo "=============================================="
	@../checkStackUsage.py $< . 2>&1 | \
		grep -E '(func|foo|bar|main) '
	@echo "=============================================="
	@echo "1. The output shown above must contain 4 lines"
	@echo "2. 'foo' and 'bar' must both be calling 'func'"
	@echo "   *but with different stack sizes in each*."
	@echo "3. 'main' must be using the largest 'func'"
	@echo "   (i.e. be going through 'bar')"
	@echo "4. The reported sizes must properly accumulate"
	@echo "=============================================="

${TARGET}:	${OBJ}
	${CC} -o $@ ${CFLAGS} $^

clean:
	rm -f ${OBJ} ${TARGET} *.su
