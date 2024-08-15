

all: list cscope_index


list:
	@cd ..; find . -name '*.c' -o -name '*.h' > cscope/cscope.files

cscope_index:
	@cd ..;                                       \
	CSCOPE=`which cscope`;                        \
	if [[ "$$CSCOPE" != "" ]]; then               \
	    $$CSCOPE -q -R -b -i cscope/cscope.files; \
	fi

clean:
	@rm -rf ../cscope*out
