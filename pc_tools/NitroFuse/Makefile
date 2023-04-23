CXX			:=	g++
CXX_FLAGS	:=	-std=gnu++17 -O3 -D_FILE_OFFSET_BITS=64
LD_FLAGS	:=	-lfuse
BUILD		:=	$(CURDIR)/build
OBJ_DIR		:=	$(BUILD)/obj
OUT_DIR		:=	$(BUILD)/bin
TARGET		:=	nitrofuse

LIBNEDIT	:=	$(CURDIR)/../../libnedit
COMMON		:=	$(CURDIR)/../common
INCLUDE		:=	-I$(LIBNEDIT)/include/ \
				-I$(COMMON)/include/
SRC			:=	$(shell find $(LIBNEDIT)/source/ -type f -name '*.cpp') \
				$(shell find $(COMMON)/source/ -type f -name '*.cpp') \
				$(shell find $(CURDIR)/source/ -type f -name '*.cpp')

OBJECTS		:=	$(SRC:%.cpp=$(OBJ_DIR)/%.o)

all: build $(OUT_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	@echo $(notdir $<)
	@$(CXX) $(CXX_FLAGS) $(INCLUDE) -c $< -o $@

$(OUT_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	@$(CXX) -o $(OUT_DIR)/$(TARGET) $^ $(LD_FLAGS)
	@echo built - $(OUT_DIR)/$(TARGET)

.PHONY: all build clean

build:
	@mkdir -p $(OUT_DIR)
	@mkdir -p $(OBJ_DIR)

clean:
	@rm -rf $(BUILD)/