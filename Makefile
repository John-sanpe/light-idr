# SPDX-License-Identifier: GPL-2.0-or-later
flags = -g -O2 -std=gnu11 -Wall -Werror -I src/
head  = src/idr.h src/list.h src/rbtree.h
obj   = src/idr.o src/rbtree.o
demo  = examples/simple

all: $(demo)

%.o:%.c $(head)
	@ echo -e "  \e[32mCC\e[0m	" $@
	@ gcc -o $@ -c $< $(flags)

$(demo): $(obj) $(addsuffix .c,$(demo))
	@ echo -e "  \e[34mMKELF\e[0m	" $@
	@ gcc -o $@ $@.c $(obj) $(flags)

clean:
	@ rm -f $(obj) $(demo)
