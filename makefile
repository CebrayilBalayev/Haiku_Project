server=server.o
client=client.o
test=test.o

server:$(server)
	gcc $^ -o $@ -pthread

client:$(client)
	gcc $^ -o $@ 

tests:$(test)
	gcc $^ -o $@ -lcunit -pthread

%.o : %.c
	gcc -c $^ -o $@

clean:
	rm *.o