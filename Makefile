DIR=.build

make: $(DIR)
	$(MAKE) -C $(DIR)

$(DIR):
	mkdir -p ./$(DIR)

debug: $(DIR)
	cd $(DIR) && cmake -DWITHTEST:BOOL=ON -DCMAKE_BUILD_TYPE=Debug ..
	$(MAKE) -C $(DIR)

release: $(DIR)
	cd $(DIR) && cmake -DWITHTEST:BOOL=ON -DCMAKE_BUILD_TYPE=Release ..
	$(MAKE) -C $(DIR)

debug_notest: $(DIR)
	cd $(DIR) && cmake -DCMAKE_BUILD_TYPE=Debug ..
	$(MAKE) -C $(DIR)

release_notest: $(DIR)
	cd $(DIR) && cmake  -DCMAKE_BUILD_TYPE=Release ..
	$(MAKE) -C $(DIR)

test:
	$(MAKE) -C $(DIR) tests

clean:
	$(MAKE) -C $(DIR) clean

uni:
	cat ./include/mitosha.h > ./mitosha.h
	cat ./src/*.c > ./mitosha.c

drop: clean
	rm -rf ./$(DIR)
	rm -f ./lib/*
	rm -f ./mitosha.c ./mitosha.h


