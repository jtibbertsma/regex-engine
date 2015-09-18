compile  = gcc -std=gnu99 -O0 -Wall -Wextra -g
%compile = gcc -std=gnu99 -O3
objects  = class.o bts.o atom.o core.o parser.o factory.o tokens.o shre_errno.o util.o shre.o clist.o range.o obhash.o u8_translate.o

all : regex

regex  : ${objects} main.o
	${compile} -o regex ${objects} main.o
   
class.o        : class.c class.h util.h hooks.h
	${compile} -c $<

bts.o        : bts.c bts.h range.h
	${compile} -c $<

atom.o      : atom.c atom.h class.h bts.h core.h range.h util.h
	${compile} -c $<

core.o       : core.c core.h atom.h class.h bts.h range.h util.h
	${compile} -c $<

parser.o     : parser.c parser.h shre_errno.h tokens.h class.h util.h obhash.h u8_translate.h
	${compile} -c $<

factory.o    : factory.c factory.h tokens.h class.h core.h atom.h bts.h range.h clist.h
	${compile} -c $<

tokens.o     : tokens.c tokens.h class.h u8_translate.h hooks.h
	${compile} -c $<

shre_errno.o : shre_errno.c shre_errno.h
	${compile} -c $<

shre.o       : shre.c core.h class.h bts.h parser.h tokens.h factory.h shre.h util.h range.h obhash.h
	${compile} -c $<

util.o       : util.c util.h
	${compile} -c $<

range.o      : range.c range.h
	${compile} -c $<

clist.o      : clist.c clist.h atom.h core.h class.h range.h bts.h
	${compile} -c $<

obhash.o     : obhash.c obhash.h util.h hooks.h
	${compile} -c $<

u8_translate.o    : u8_translate.c u8_translate.h util.h
	${compile} -c $<

main.o       : main.c shre.h shre_errno.h
	${compile} -c $<

library      : shininglib.so

shininglib.so   : ${objects}
	${compile} -shared -o shininglib.so ${objects}

puke         : library puke.c shre.h shre_errno.h
	${compile} -o puke puke.c shrelib.so

classtest.c : ctinp.py
	./ctinp.py > classtest.c

classtest    : class.o classtest.c class.h util.h hooks.h
	${compile} -o classtest classtest.c class.o

clean      :
	rm *.o
	rm regex