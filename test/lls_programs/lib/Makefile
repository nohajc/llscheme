ifdef DEBUG
BIN = ../../../bin/Debug
EXTRAFLAGS = -fsanitize=address
else
BIN = ../../../bin/Release
EXTRAFLAGS =
endif

SCMC = $(BIN)/schemec
LD = clang
LDFLAGS = -L$(BIN) -Wl,-R -Wl,"$(BIN)" -lllscmrt $(EXTRAFLAGS)

EXT=scm
TARGETS=$(shell ls *.$(EXT) | sed 's/\.scm/\.so/'g)

all: $(TARGETS)

%.so: %.o
	$(LD) -shared $< -o $@ $(LDFLAGS)

%.o: %.scm
	$(SCMC) $< -b lib

clean:
	rm $(TARGETS) || true
