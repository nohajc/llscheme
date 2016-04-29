ifdef DEBUG
BIN = ../../bin/Debug
EXTRAFLAGS = -fsanitize=address
else
BIN = ../../bin/Release
EXTRAFLAGS =
endif

SCMC = $(BIN)/schemec
LD = clang
LDFLAGS = -L$(BIN) -Wl,-R,"$(BIN)",-R,'.' -lllscmrt $(EXTRAFLAGS)

EXT=scm
TARGETS=$(shell ls *.$(EXT) | xargs -L1 -I % basename % .$(EXT))

all: $(TARGETS)

%: %.o
	# Parse the sources, look for "require", extract the library names
	# and construct the corresponding linker flags (-L, -l, -Wl,-R)
	EXTRALIBS=`sed -n 's/^(require "\([^"]*\)")/\1/p' $($<_SRC)` ;\
	[ -n "$$EXTRALIBS" ] && LIBDIRS=`echo "$$EXTRALIBS" | xargs -L1 dirname | uniq | sed 's/^/-L.\//'` && \
	LIBNAMES=`echo "$$EXTRALIBS" | xargs -L1 basename | uniq | sed 's/^/-l:/' | sed 's/$$/\.so/'` && \
	RPATHS=`echo "$$EXTRALIBS" | xargs -L1 dirname | uniq | sed 's/^/-Wl,-R,\\$$ORIGIN\//'`; \
	$(LD) $< -o $@ $(LDFLAGS) $$LIBDIRS $$LIBNAMES $$RPATHS

%.o: %.scm
	$(eval $@_SRC=$<)
	$(SCMC) $<

clean:
	rm $(TARGETS) || true
