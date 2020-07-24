#make file for myshell
myshell: main.o shell.o command.o builtin_func.o shell_env.o status.o pv.o token.o command.o
	gcc main.o shell.o builtin_func.o shell_env.o status.o pv.o token.o command.o -o myshell 

main.o: main.c shell.h builtin_func.h shell_env.h status.h pv.h 
	gcc -c main.c 

shell.o: shell.h shell.c command.h 
	gcc -c shell.c 

command.o: command.h command.c pv.h status.h builtin_func.h
	gcc -c command.c 

builtin_func.o: builtin_func.h builtin_func.c shell_env.h status.h 
	gcc -c builtin_func.c 

shell_env.o: shell_env.h shell_env.c status.h shell_env.h
	gcc -c shell_env.c 

status.o: status.h status.c shell.h
	gcc -c status.c 

pv.o: pv.h pv.c status.h 
	gcc -c pv.c 

token.o: token.h token.c
	gcc -c token.c

clean: 
	rm *.o
