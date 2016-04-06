ifdef DEBUG
BIN = ../../bin/Debug
else
BIN = ../../bin/Release
endif

SCMC = $(BIN)/schemec

all: scmlib.o

scmlib.o: scmlib.scm
	$(SCMC) $< -b lib

clean:
	rm scmlib.o || true
