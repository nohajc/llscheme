SCMC=../../bin/Debug/schemec
LD=clang
LDFLAGS=-L../../bin/Debug -fsanitize=address

EXT=scm
TARGETS=$(shell ls *.$(EXT) | xargs -L1 -I % basename % .$(EXT))

all: $(TARGETS)

%: %.o
	clang $< -o $@ $(LDFLAGS) -lllscmrt

%.o: %.scm
	$(SCMC) $<

clean:
	rm $(TARGETS) || true
