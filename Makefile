LD ?= ld
CC ?= cc
CFLAGS := -Wall -Wextra -Werror -Winline -fPIC
RANLIB ?= ranlib

LIBS := zmq

ifdef DEBUG
	CFLAGS += -g
endif


BUILD_DIR := build

OBJ_DIR := $(BUILD_DIR)/obj
DOC_DIR := $(BUILD_DIR)/docs

SRC_DIR := src

SRC_FILES := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/**/*.c)

OBJS := $(SRC_FILES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

DIRS := $(sort $(dir $(OBJS)))

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c -o $@ $<

.PHONY: clean all shared static include

all: shared static include

shared: $(BUILD_DIR)/libmongrel2.so

include:

clean:
	rm -rf $(BUILD_DIR)/*

$(BUILD_DIR)/libmongrel2.so: $(OBJS)
	$(LD) --export-dynamic -shared -lc $(addprefix -l,$(LIBS)) -o $@ $^

$(OBJS): | dirs $(DIRS)

.PHONY: dirs
dirs:
	@echo "Making Build Directories"
$(DIRS):
	mkdir -p $@

