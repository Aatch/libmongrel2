LD ?= ld
CC ?= cc
CFLAGS := -Wall -Wextra -Werror -Winline -fPIC
RANLIB ?= ranlib

LIBS := zmq

BUILD_DIR := build

OBJ_DIR := $(BUILD_DIR)/obj
DOC_DIR := $(BUILD_DIR)/docs

SRC_DIR := src

SRC_FILES := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/**/*.c)

OBJS := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

DIRS := $(sort $(dir $(OBJS)))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -g -I$(SRC_DIR) -c -o $@ $<

.PHONY: clean all shared static include

all: shared static include

shared: $(BUILD_DIR)/libmongrel2.so

static: $(BUILD_DIR)/libmongrel2.a

include:

clean:
	rm -rf $(BUILD_DIR)/*

$(BUILD_DIR)/libmongrel2.so: $(OBJS)
	$(LD) --export-dynamic -shared -lc $(addprefix -l,$(LIBS)) -o $@ $^

$(BUILD_DIR)/libmongrel2.a: $(OBJS)
	$(AR) cr $@ $?
	$(RANLIB) $(BUILD_DIR)/libmongrel2.a

$(OBJS): | dirs $(DIRS)

.PHONY: dirs
dirs:
	@echo "Making Build Directories"
$(DIRS):
	mkdir -p $@

