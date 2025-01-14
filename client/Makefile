CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -pthread
TARGET = client
SRCS = main.c read.c write.c list.c create.c delete.c copy.c connection.c operations.c info.c second_connection.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

# Compile source files to object files
%.o: %.c header.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

rebuild: clean all

.PHONY: all clean rebuild