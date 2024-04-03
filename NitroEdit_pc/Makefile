
export NEDIT_MAJOR	:=	0
export NEDIT_MINOR	:=	1
export NEDIT_MICRO	:=	0
export NEDIT_BUGFIX	:=	0
export NEDIT_VERSION	:=	$(NEDIT_MAJOR).$(NEDIT_MINOR).$(NEDIT_MICRO).$(NEDIT_BUGFIX)
export NEDIT_MODULES_DIR	:=	modules
export NEDIT_OUTPUT_PATH	:=	$(CURDIR)/bin
export NEDIT_OUTPUT_MODULES_PATH	:=	$(NEDIT_OUTPUT_PATH)/modules

BUILTIN_MODULES	:=	Bmg

###################################################################

BUILTIN_MODULE_BUILD_TARGETS	:=	$(foreach target,$(BUILTIN_MODULES),$(target).build)
BUILTIN_MODULE_CLEAN_TARGETS	:=	$(foreach target,$(BUILTIN_MODULES),$(target).clean)

.PHONY: all clean base libnedit nedit

all: base libnedit nedit $(BUILTIN_MODULE_BUILD_TARGETS)

clean: $(BUILTIN_MODULE_CLEAN_TARGETS)
	@echo Cleaning libnedit...
	@cmake --build libnedit/build --target clean
	@echo Cleaning NitroEdit...
	@cmake --build NitroEdit/build --target clean
	@rm -rf bin

base:
	@mkdir -p $(NEDIT_OUTPUT_PATH)
	@mkdir -p $(NEDIT_OUTPUT_MODULES_PATH)

libnedit:
	@echo Building libnedit...
	@mkdir -p libnedit/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S libnedit -B libnedit/build
	@cmake --build libnedit/build

nedit:
	@echo Building NitroEdit...
	@mkdir -p NitroEdit/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S NitroEdit -B NitroEdit/build
	@cmake --build NitroEdit/build

%.build:
	@echo Building builtin module '$(basename $@)'...
	@mkdir -p modules/$(basename $@)/build
	@cmake -DCMAKE_BUILD_TYPE=Release -S modules/$(basename $@) -B modules/$(basename $@)/build
	@cmake --build modules/$(basename $@)/build

%.clean:
	@echo Cleaning builtin module '$(basename $@)'...
	@cmake --build modules/$(basename $@)/build --target clean
