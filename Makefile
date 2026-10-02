TARGET_EXE := render
BUILD_DIR := ./build
SRC_DIR := ./src
LIB_DIR := ./lib

STB_PATH := $(LIB_DIR)/stb

NLOHMANN_JSON_PATH := $(LIB_DIR)/json/single_include
YAML_CPP_PATH := $(LIB_DIR)/yaml-cpp
YAML_CPP_BUILD_DIR := $(YAML_CPP_PATH)/build
YAML_CPP_INC_DIR := $(YAML_CPP_PATH)/include
YAML_CPP_LIBYAMLCPP_PATH := $(YAML_CPP_BUILD_DIR)/libyaml-cpp.a

TINYOBJLOADER_PATH := $(LIB_DIR)/tinyobjloader

CC := g++
COMMON_FLAGS := -O3 -g -lpthread
CFLAGS := -Wall -Wextra
CPPFLAGS := -MMD -MP -I$(STB_PATH) -I$(NLOHMANN_JSON_PATH) -I$(TINYOBJLOADER_PATH) -I$(YAML_CPP_INC_DIR)
LDFLAGS := --gc-sections

SOURCES := $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/*/*.cpp) # Shell "find" sucks on Windows, so we're doing this
OBJS := $(SOURCES:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d) # Generate sub-makefiles for each C source

# Turn LDFLAGS into -Wl,[flag],[flag]... to pass to GCC
space := $() $()
comma := ,
LDFLAGS := -Wl,$(subst $(space),$(comma),$(LDFLAGS))

ifneq ($(wildcard ./venv/Scripts/activate),)
  $(eval PYTHON := ./venv/Scripts/python.exe)
  $(eval PIP := ./venv/Scripts/pip.exe)
else ifneq ($(wildcard ./venv/bin/activate),)
  $(eval PYTHON := ./venv/bin/python3)
  $(eval PIP := ./venv/bin/pip3)
else
  $(info Python venv not found, generating...)
  $(shell python3 -m venv ./venv)

  ifneq ($(wildcard ./venv/Scripts/activate),)
    $(eval PYTHON := ./venv/Scripts/python.exe)
    $(eval PIP := ./venv/Scripts/pip.exe)
  else ifneq ($(wildcard ./venv/bin/activate),)
    $(eval PYTHON := ./venv/bin/python3)
    $(eval PIP := ./venv/bin/pip3)
  endif
endif

all: $(BUILD_DIR)/$(TARGET_EXE) compiledb

# Link C sources into final executable
$(BUILD_DIR)/$(TARGET_EXE): $(OBJS) libyaml-cpp
	$(CC) $(COMMON_FLAGS) $(LDFLAGS) $(OBJS) $(YAML_CPP_LIBYAMLCPP_PATH) -o $@

# Build C sources
$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(COMMON_FLAGS) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Build ASM sources
$(BUILD_DIR)/%.s.o: %.s
	mkdir -p $(dir $@)
	$(CC) $(COMMON_FLAGS) $(CPPFLAGS) $(DFU_CPPFLAGS) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

libyaml-cpp:
	mkdir -p $(YAML_CPP_BUILD_DIR)
	cd $(YAML_CPP_BUILD_DIR) && cmake .. && make

# Generate ./build/compile_commands.json using compiledb
compiledb: $(BUILD_DIR)/$(TARGET_EXE)
	mkdir -p $(BUILD_DIR)
	$(PYTHON) -m compiledb -n -o $(BUILD_DIR)/compile_commands.json make

documentation:
# Make initial doc, make bibliography, put everything together, then do it again for good measure
# https://tex.stackexchange.com/questions/204291/bibtex-latex-compiling
	pdflatex -halt-on-error -output-directory=docs -aux-directory=docs ./docs/main.tex
	bibtex -include-directory=docs ./docs/main.aux
	pdflatex -halt-on-error -output-directory=docs -aux-directory=docs ./docs/main.tex
	pdflatex -halt-on-error -output-directory=docs -aux-directory=docs ./docs/main.tex

setup:
	mkdir -p ./.vscode
	cp ./scripts/vscode/* ./.vscode
	$(PIP) install compiledb

clean:
	rm -r $(BUILD_DIR)

.PHONY: all compiledb clean
-include $(DEPS)
