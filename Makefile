# Shortcuts for the CMake presets. Pass PRESET to pick another one, for example make PRESET=wiiu.
PRESET ?= linux

.PHONY: all configure test format clean

all: configure
	cmake --build --preset $(PRESET)

configure:
	cmake --preset $(PRESET)

test:
	ctest --preset $(PRESET)

format:
	clang-format -i $$(find libs lua/src lua/include test -name '*.h' -o -name '*.cpp')

clean:
	rm -rf build/$(PRESET)
