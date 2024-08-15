
.PHONY: all



all:
	@for D in fs cscope; do $(MAKE) -C $$D all; done



clean:
	@for D in fs cscope; do $(MAKE) -C $$D clean; done


