
# 
# This is a fairly generic c99 makefile
# 

target  = nonet
opt     = 0
headers = $(wildcard src/*.h)
warns   = -Wall -Wextra -Werror -Wfatal-errors
flags   = -O$(opt) -std=gnu99
deps    = 
incldir = -I./src/
allc    = $(patsubst src/%.c,%,$(wildcard src/*.c))
ofiles  = $(addprefix bld/,$(addsuffix .o,$(allc)))
cfiles  = $(addprefix src/,$(addsuffix .c,$(allc)))
efiles  = $(addprefix bld/,$(addsuffix .E$(opt),$(allc)))
sfiles  = $(addprefix bld/,$(addsuffix .S$(opt),$(allc)))

$(target): Makefile $(headers) $(ofiles)
	gcc $(ofiles) $(deps) -o $@

.PHONY: e
e: Makefile $(headers) $(efiles)

.PHONY: s
s: Makefile $(headers) $(sfiles)

bld/%.o: src/%.c Makefile $(headers)
	gcc -c $(warns) $(incldir) $(flags) $< -o $@

bld/%.E$(opt): src/%.c Makefile $(headers)
	gcc -E $(warns) $(incldir) $(flags) $< -o $@

bld/%.S$(opt): src/%.c Makefile $(headers)
	gcc -S $(warns) $(incldir) $(flags) $< -o $@

.PHONY: clean
clean:
	rm -rf bld/*
	rm -f "$(target)"
