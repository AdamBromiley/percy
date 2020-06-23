.SUFFIXES:
.SUFFIXES: .c .h .o .so




# Output dynamic library
_OUT = libpercy.so
OUTDIR = .
OUT = $(OUTDIR)/$(_OUT)

# Source code
_SRC = parser.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = parser.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = parser.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Include directories
_IDIRS = include
IDIRS = $(patsubst %,-I%,$(_IDIRS))




# Compiler name
CC = gcc

# Compiler optimisation options
COPT = -O2

# Compiler options
CFLAGS = $(IDIRS) $(COPT) -fPIC -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker optimisation options
LDOPT = -O2

# Linker options
LDFLAGS = $(LDLIBS) $(LDOPT) -shared




.PHONY: all
all: $(OUT)




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Compile object files into dynamic library
$(OUT): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $(OUT)




.PHONY: clean
# Remove object files and dynamic library
clean:
	rm -f $(OBJS) $(OUT)