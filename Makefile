CC = gcc
FLAGS = -Wall -Wextra -pthread -g
ERLC = erlc


C_FILES = C/agent.c \
          $(wildcard C/network/*.c) \
          $(wildcard C/scheduler/*.c) \
          $(wildcard C/structures/*.c)

# Genera los archivos .o a partir de los .c
C_OBJS = $(C_FILES:.c=.o)

# El ejecutable final se llama agent y se encuentra en la carpeta C
C_EXEC = C/agent

ERL_FILES = $(wildcard Erlang/*.erl)
ERL_BEAMS = $(patsubst Erlang/%.erl,Erlang/%.beam,$(ERL_FILES))

# Le dice a make que estos son comandos
.PHONY: all c erlang clean

# El comando por defecto es compilar tanto el código en C como el de Erlang
all: c erlang

# Compila el código en C
c: $(C_EXEC)

# Compila el ejecutable de C a partir de los objetos
$(C_EXEC): $(C_OBJS)
	$(CC) $(FLAGS) -o $@ $^ 

# Compila cada archivo .c a su correspondiente .o
%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@

# Compila el código en Erlang
erlang: $(ERL_BEAMS)

# Compila cada archivo .erl a su correspondiente .beam
Erlang/%.beam: Erlang/%.erl
	$(ERLC) -o Erlang $<

# Limpia los archivos compilados y los logs
clean:
	rm -f $(C_OBJS) $(C_EXEC)
	rm -f Erlang/*.beam
	rm -f log_node*.txt Erlang/logs.txt