
.PHONY: all



all:
	@for D in fs cscope; do make -C $$D all; done


clean:
	@for D in fs cscope; do make -C $$D clean; done


