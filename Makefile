NAME := wsi-tv
SRC_DIR := srcx
BIN_DIR := bin
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
HDR_FILES := $(wildcard $(SRC_DIR)/*.h)
LDFLAGS := -lopenslide -lm
CXXFLAGS := -Wall -Wextra -pedantic -std=c99 -O3

$(NAME): $(SRC_FILES) $(HDR_FILES) Makefile
	$(CC) $(SRC_FILES) $(HDR_FILES) -o $(BIN_DIR)/$(NAME) $(LDFLAGS) $(CXXFLAGS)

clean:
	rm $(BIN_DIR)/$(NAME)
