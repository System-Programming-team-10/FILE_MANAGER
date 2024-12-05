# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lncurses -lpanel

# Source files and header files
SRCS = main.c display.c file.c gui.c
HEADERS = display.h file.h gui.h

# Output executable
TARGET = file_manager

# Object files
OBJS = $(SRCS:.c=.o)

# Build the program
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compile .c files to .o files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)
