# NOTE(mal): Flags for both gcc and clang
OPTIMIZATION_LEVEL = -O0
DEBUG = -g
WARNING_FLAGS = -Wall -Wextra -Wpedantic -Werror
DEPENDENCY_GENERATION = -MMD -MP
LANGUAGE_STANDARD = -std=gnu99
CFLAGS = $(WARNING_FLAGS) $(OPTIMIZATION_LEVEL) $(DEBUG) $(DEPENDENCY_GENERATION) $(LANGUAGE_STANDARD)

CC = clang
ACCEPTED_WARNINGS += -Wno-missing-braces -Wno-missing-field-initializers -Wno-unused-function -Wno-unused-variable -Wno-unused-parameter

BUILD_DIR = build
TARGETS = $(BUILD_DIR) $(BUILD_DIR)/test_file_handles

all: $(TARGETS)

# NOTE(mal): Auto-generated dependency rules (from DEPENDENCY_GENERATION)
-include $(wildcard $(BUILD_DIR)/*.d)

$(BUILD_DIR):
	mkdir $@

$(BUILD_DIR)/test_file_handles: src/test_file_handles.c
	$(CC) $(CFLAGS) $(ACCEPTED_WARNINGS) $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)
