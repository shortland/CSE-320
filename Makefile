CC := gcc
SRCD := src
TSTD := tests
BLDD := build
BIND := bin
INCD := include

ALL_SRCF := $(shell find $(SRCD) -type f -name *.c)
ALL_OBJF := $(patsubst $(SRCD)/%,$(BLDD)/%,$(ALL_SRCF:.c=.o))
FUNC_FILES := $(filter-out build/main.o, $(ALL_OBJF))

TEST_SRC := $(shell find $(TSTD) -type f -name *.c)

INC := -I $(INCD)

CFLAGS := -Wall -Werror -Wextra -Wno-variadic-macros -ansi -pedantic \
          -DLINUX -D_DEFAULT_SOURCE -DENGLISH
COLORF := -DCOLOR
DFLAGS := -g -DDEBUG -DCOLOR
#PRINT_STAMENTS := -DERROR -DSUCCESS -DWARN -DINFO

STD := -std=c99
TEST_LIB := -lcriterion
LIBS :=

CFLAGS += $(STD)

#Uncomment the following to use the "clear_screen" function from the
#curses library:
#CFLAGS += -DTERMINFO
#LIBS += -lcurses

EXEC := rolo
TEST := $(EXEC)_tests
ROLOLIB = \"$(shell pwd)/helplib\"
TOOLDIR = ./toolsdir

CFLAGS += -DROLOLIB=$(ROLOLIB) -I $(INCD)/$(TOOLDIR)

.PHONY: clean all setup debug

all: setup $(BIND)/$(EXEC) $(BIND)/$(TEST)

debug: CFLAGS += $(DFLAGS) $(PRINT_STAMENTS) $(COLORF)
debug: all

setup: $(BIND) $(BLDD)
$(BIND):
	mkdir -p $(BIND) $(BIND)/$(TOOLDIR)
$(BLDD):
	mkdir -p $(BLDD) $(BLDD)/$(TOOLDIR)

$(BIND)/$(EXEC): $(ALL_OBJF)
	$(CC) $^ -o $@ $(LIBS)

$(BIND)/$(TEST): $(FUNC_FILES) $(TEST_SRC)
	$(CC) $(CFLAGS) $(INC) $(FUNC_FILES) $(TEST_SRC) $(TEST_LIB) $(LIBS) -o $@

$(BLDD)/%.o: $(SRCD)/%.c
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	rm -rf $(BLDD) $(BIND) *.out

.PRECIOUS: $(BLDD)/*.d
-include $(BLDD)/*.d
