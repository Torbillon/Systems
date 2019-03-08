objects = arg_parse.o builtin.o ush.o


ush : $(objects)
	cc -o ush $(objects)
ush.o: ush.c


builtin.o: builtin.c builtin.h
arg_parse.o: arg_parse.c arg_parse.h


.PHONY: clean
clean:
	-rm ush $(objects)
