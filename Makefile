DIR := .build

all: debug

$(DIR):
	mkdir -p $(DIR)

define cmake_configure
	cmake -S . -B $(DIR) $(1)
endef

debug: $(DIR)
	$(call cmake_configure, -DWITHTEST=ON -DCMAKE_BUILD_TYPE=Debug)
	$(MAKE) -C $(DIR)

release: $(DIR)
	$(call cmake_configure, -DWITHTEST=ON -DCMAKE_BUILD_TYPE=Release)
	$(MAKE) -C $(DIR)

debug_notest: $(DIR)
	$(call cmake_configure, -DCMAKE_BUILD_TYPE=Debug)
	$(MAKE) -C $(DIR)

release_notest: $(DIR)
	$(call cmake_configure, -DCMAKE_BUILD_TYPE=Release)
	$(MAKE) -C $(DIR)

test:
	$(MAKE) -C $(DIR) tests

clean:
	$(MAKE) -C $(DIR) clean

drop: clean
	rm -rf $(DIR)
	rm -f ./lib/*
	rm -f ./mitosha.c ./mitosha.h

uni:
	cat ./include/mitosha.h > ./mitosha.h
	cat ./src/*.c > ./mitosha.c

format:
	git diff --name-only --diff-filter=ACMRTUXB | grep -E '\.(c|h|cpp|hpp|cc|cxx)$$' | xargs --no-run-if-empty clang-format -i

format-all:
	find . \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' -o -name '*.cc' -o -name '*.cxx' \) -exec clang-format -i {} +

.PHONY: all debug release debug_notest release_notest test clean drop uni format format-all

