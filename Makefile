# ======================================================================
# Root Makefile: Builds all three subprojects in the correct order.
# Usage:
#	make 			build everything
#	make init		runs bear -- make on every subfolder to prepare intellisense
#   make tetris 	build corestack + tetris only
#   make bomberman 	build corestack + bomberman only
#   make test 		run all tests across all projects
#   make clean 		wipe all build artifacts
#	make distclean 	wipe everything including dependencies downloaded
#	make run-server [tetris|bomberman]	launch that project's server
#	make run-client [tetris|bomberman]	launch that project's client
# ======================================================================

.PHONY: all corestack tetris bomberman test clean distclean lint gen-auth run-server run-client

all: corestack tetris bomberman

# Generates auth/private_key.pem + auth/server_signed.crt (self-signed, local
# dev only) if they don't already exist. bombd won't start without these.
gen-auth:
	./scripts/generate_auth.sh

init:
	$(MAKE) -C corestack init
	$(MAKE) -C tetris init
	$(MAKE) -C bomberman init

# `tetris`/`bomberman` double as the game name passed to run-server/run-client
# (eg `make run-server bomberman`) - when used that way, don't also run their
# normal build recipe (which forces a full clean+rebuild via corestack)
ifeq (,$(filter run-server run-client,$(MAKECMDGOALS)))
corestack: clean
	$(MAKE) -C corestack

tetris: corestack
	$(MAKE) -C tetris

bomberman: corestack
	$(MAKE) -C bomberman
else
tetris bomberman: ;
endif

GAME := $(filter tetris bomberman,$(MAKECMDGOALS))

run-server:
	@if [ -z "$(GAME)" ]; then echo "Usage: make run-server [tetris|bomberman]"; exit 1; fi
	$(MAKE) -C $(GAME) run-server

run-client:
	@if [ -z "$(GAME)" ]; then echo "Usage: make run-client [tetris|bomberman]"; exit 1; fi
	$(MAKE) -C $(GAME) run-client

# Runs all tests across all 3 projects
test: all
	$(MAKE) -C corestack test
	$(MAKE) -C tetris    test
	$(MAKE) -C bomberman test

# Remove all compiled output
clean:
	$(MAKE) -C corestack clean
	$(MAKE) -C tetris    clean
	$(MAKE) -C bomberman clean

# Also remove dependencies (eg: raylib build)
distclean: clean
	$(MAKE) -C bomberman distclean

# Run clang-format (spacing & brackets) on all source files
format:
	${MAKE} -C corestack format
	${MAKE} -C tetris    format
	${MAKE} -C bomberman format

# Fix variable naming via clang-tidy, 
# then running clang-format to fix layout (spacing & brackets)
# tidy > format as renames can change line lengths
lint:
	${MAKE} -C corestack lint
	${MAKE} -C tetris    lint
	${MAKE} -C bomberman lint

