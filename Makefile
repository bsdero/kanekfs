DIRS=fs


.PHONY: all



all:
	make -C $(DIRS)


clean:
	make -C $(DIRS) clean 

