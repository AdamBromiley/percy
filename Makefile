.SUFFIXES:
.SUFFIXES: .c .h .o .so




# Output dynamic library
_OUT = percy
OUTDIR = .
OUT = $(OUTDIR)/lib$(_OUT).so

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

# Files to compile for testing/demonstration
TOUT = percy_demo
TDIR = test
TEST = $(TDIR)/percy_demo.c $(HDIR)/parser.h




# Include directories
_IDIRS = include
IDIRS = $(patsubst %,-I%,$(_IDIRS))

# Libraries to be linked with `-l`
_LDLIBS = m
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))




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




.PHONY: all demo
all: $(OUT)
demo: $(TOUT)




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Compile object files into dynamic library
$(OUT): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $(OUT)


# Simple compile of demonstration script
$(TOUT): $(OUT)
	$(CC) $(TEST) -L$(OUTDIR) -Wl,-rpath=$(OUTDIR) -l$(_OUT) $(CFLAGS) -o $(TOUT)




.PHONY: clean-all clean clean-demo
# Remove object files and dynamic library
clean-all: clean clean-demo
clean:
	rm -f $(OBJS) $(OUT)
clean-demo:
	rm -f $(TOUT)