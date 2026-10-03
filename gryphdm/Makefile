# Copyright (C) 2026  Ratmir Timergaliev
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://gnu.org>.

CXX ?= g++

SRC_PATH = .
BUILD_PATH = build
BIN_PATH = $(BUILD_PATH)/bin

BIN_NAME = gryphdm

SRC_EXT = cc

SOURCES = $(shell find $(SRC_PATH) -name '*.$(SRC_EXT)' | sort -k 1nr | cut -f2-)
OBJECTS = $(SOURCES:$(SRC_PATH)/%.$(SRC_EXT)=$(BUILD_PATH)/%.o)
DEPS = $(OBJECTS:.o=.d)

COMPILE_FLAGS = -std=c++11 -Wall -Wextra -g
INCLUDES = -I include/ -I /usr/local/include
LIBS = -lncurses

PREFIX ?= /usr/local

.PHONY: default_target
default_target: release

.PHONY: release
release: export CXXFLAGS := $(CXXFLAGS) $(COMPILE_FLAGS)
release: dirs
	@$(MAKE) all

.PHONY: dirs
dirs:
	@echo "Creating directories"
	@mkdir -p $(dir $(OBJECTS))
	@mkdir -p $(BIN_PATH)

.PHONY: clean
clean:
	@echo "Deleting directories"
	@$(RM) -r $(BUILD_PATH)
	@$(RM) -r $(BIN_PATH)

.PHONY: all
all: $(BIN_PATH)/$(BIN_NAME)

$(BIN_PATH)/$(BIN_NAME): $(OBJECTS)
	@echo "LD $@"
	@$(CXX) $(OBJECTS) -o $@ ${LIBS}

-include $(DEPS)

$(BUILD_PATH)/%.o: $(SRC_PATH)/%.$(SRC_EXT)
	@echo "CC $< -> $@"
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -MP -MMD -c $< -o $@

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp build/bin/$(BIN_NAME) $(DESTDIR)$(PREFIX)/bin/$(BIN_NAME)
	
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(BIN_NAME)

