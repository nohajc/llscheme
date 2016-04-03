BIN=../../bin/Debug
SCMC=$(BIN)/schemec
LD=clang
LDFLAGS=-L$(BIN) -lllscmrt -lgc -fsanitize=address

EXT=scm
TARGETS=$(shell ls *.$(EXT) | xargs -L1 -I % basename % .$(EXT))

all: $(TARGETS)

%: %.o
	clang $< -o $@ $(LDFLAGS)

%.o: %.scm
	$(SCMC) $<

clean:
	rm $(TARGETS) || true
