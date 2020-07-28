TARGET := dining

OBJECT := dining.o

SOURCE := dining.c

CFLAGS += -lpthread

CC := gcc

$(TARGET) : $(OBJECT)
	$(CC) -o $(TARGET) $(OBJECT) $(CFLAGS) 

dining.o : dining.c
	$(CC) -c -o $(OBJECT) $(SOURCE)

clean:
	rm $(OBJECT) $(TARGET)
