CC = x86_64-borrrdex-gcc
LD = x86_64-borrrdex-ld
ASM = nasm

override CFLAGS := -g $(CFLAGS)
LDFLAGS = -lc

SRCDIR := src
OBJDIR := lib
BUILDDIR = bin

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

SRC = $(call rwildcard,$(SRCDIR),*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp, $(OBJDIR)/%.o, $(SRC))
DIRS = $(wildcard $(SRCDIR)/*)

all: setup $(OBJS) link

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@ echo !== Compiling $^
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

link:
	@ echo !== Linking $^
	$(LD) $(LDFLAGS) -o $(BUILDDIR)/usertest ${HOME}/sysroot/usr/lib/crt0.o $(OBJS)

setup:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(SRCDIR)
	@mkdir -p $(OBJDIR)

clean:
	rm -f $(BUILDDIR)/*
	rm -rf $(OBJDIR)
