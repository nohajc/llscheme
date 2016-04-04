ifdef DEBUG
BIN = ../../bin/Debug
EXTRAFLAGS = -fsanitize=address
else
BIN = ../../bin/Release
EXTRAFLAGS =
endif

SCMC = $(BIN)/schemec
LD = clang
LDFLAGS = -L$(BIN) -Wl,-R -Wl,"$(BIN)" -lllscmrt $(EXTRAFLAGS)

EXT=scm
TARGETS=$(shell ls *.$(EXT) | xargs -L1 -I % basename % .$(EXT))

all: $(TARGETS)

%: %.o
	$(LD) $< -o $@ $(LDFLAGS)

%.o: %.scm
	$(SCMC) $<

clean:
	rm $(TARGETS) || true
