ifneq ($(CROSS),)
	CROSS_PREFIX	:= $(CROSS)-
else
	CROSS_PREFIX	:= arm-linux-gnueabihf-
endif

CC	= $(CROSS_PREFIX)gcc
CFLAGS	= -Wall -Wextra -g
LFLAGS	= 

TARGET	= 2jcie-bu01

all: $(TARGET)

$(TARGET): main.o data_output.o sensor_data.o
		$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
		$(RM) *~ *.o $(TARGET)

%.o: %.c
		$(CC) $(CFLAGS) -c -o $@ $<

