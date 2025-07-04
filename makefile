# Define the default port number if not defined
ifndef PORT
  PORT = y
endif

# Compiler flags including the defined port
CFLAGS = -DPORT=$(PORT) -g -Wall

# Target for building the program
battle: battle.c
	gcc $(CFLAGS) -o battle battle.c

# Clean up object files and executable
clean:
	rm -f battle
