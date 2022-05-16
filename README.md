The main is to create a Haiku Generator Server using C language which will print the Haiku with different categories depending on signals. There are two categories of Haikus Japanese(SIGINT) and Western(SIGQUIT). 
Used technologies : 
 1. Message Queues
 2. Shared Memory
 3. Threads
 4. Signals

first you need to start a server process then client

make server
./server
to run the server

make client
./client
to run client


----------------------------------------------------------
make tests
./tests
run unitests


