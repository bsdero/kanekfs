

all: list cscope_index


list:
	@cd ..; find . -name '*.c' -o -name '*.h' > cscope/cscope.files

cscope_index:
	@cd ..; cscope -q -R -b -i cscope/cscope.files

clean:
	@rm -rf ../cscope*out
