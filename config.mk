# =============================================================================
# Shared configuration for tetris and bomberman
# Single source of truth for any applications that depend on corestack!
#
# Include this at the top of each subproject Makefile:
# 	include ../config.mk
# =============================================================================
# Path from any subproject (tetris/ or bomberman/) up to the corestack folder
CORESTACK_DIR := ../corestack

# Where the compiler looks for corestack's public header files (#include)
CS_INC := $(CORESTACK_DIR)/include

# Where the linker looks for corestack's compiled static libraries (.a files)
CS_LIB := $(CORESTACK_DIR)/build/lib

# Auto-detect which corestack libraries to link against by scanning
# corestack's SOURCE directories (not the build folder, so this works
# even before corestack has been compiled for the first time)
# Disclaimer: The following is generated w/ the help of Claude :(
#
# How it works:
#   $(wildcard .../src/*/)       list every subdirectory under corestack/src/
#   $(patsubst %/,%,...)         strip the trailing slash from each path
#   $(notdir ...)                strip the leading path, leaving just dir names
#                                e.g. ../corestack/src/libcssh → libcssh
#   $(patsubst lib%,-l%,...)     convert lib prefix → -l flag:
#                                libcssh → -lcssh, libhtttp → -lhtttp, etc.
#
# Convention: every library directory under corestack/src/ MUST start with
# 'lib' (e.g. libcssh/, libutils/). This is what makes the patsubst work

# Subdirectory libs  e.g. src/lib/libcssh/
_CS_LIB_DIRS  := $(patsubst %/,%,$(wildcard $(CORESTACK_DIR)/src/lib/*/))
_CS_DIR_LIBS  := $(patsubst lib%,-l%,$(notdir $(_CS_LIB_DIRS)))

# Flat file libs  e.g. src/lib/libeventbus.c
_CS_FLAT_SRCS := $(wildcard $(CORESTACK_DIR)/src/lib/*.c)
_CS_FLAT_LIBS := $(patsubst lib%,-l%,$(notdir $(_CS_FLAT_SRCS:.c=)))

CS_LDLIBS := $(_CS_DIR_LIBS) $(_CS_FLAT_LIBS) -lcrypto

# Compiler flags shared across all projects
#
# -Wall -Wextra   	enable most useful compiler warnings
# -g            	embed debug symbols for gdb/valgrind
# -pthread      	tell the compiler (and linker) to enable POSIX thread support.
#                 	Needed for pthreads, mutexes, and condition vars.
# -lcrypto      	link against libcrypto for SSL/TLS
COMMON_CFLAGS := -Wall -Wextra -g -pthread