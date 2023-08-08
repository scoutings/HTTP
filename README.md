## Assignment 2

This program is an HTTP server that accepts HTTP requests (version 1.1) with methods GET, PUT, and APPEND. This server continously runs until it is told to stop and responds to clients with apropriate response codes. Additional functionality now includes an audit log which produces imformation about each request. 

## How it Works

This project makes uses of layering, abstraction, and hierarchy to organize code.

For each request given to the server the following process is followed by the server:
- Receive the request
    - Read to the socket until we find a "\r\n\r\n" in the buffer
- Parse the received buffer
    - Use Regex to distect the message into a request struct holding each seperated information
- Process the request
    - If the request is invalid respond accordingly
    - Check the method given and process accordingly
        - GET: Respond with a response containing the contents of the file
        - PUT: Respond with a response code according to how writing to the file went
        - APPEND: Respond with a response code according to how appending to the file went
- Free up any memory used during the connection

## Multithreading

This server takes an integer as an argument (thread) which determines the amount of threads that the sever will run on. This sever contains two different types of threads:

- Dispatcher thread:
    - There will always only be one dispatcher thread (this thread runs in main())
    - The purpose of this thread is to delegate takes onto worker threads

- Worker thread:
    - There will be thread amount of worker threads
    - The purpose of these threads are to actually process the requests sent to the server

These threads communicate with eachother through a bounded buffer implemented as a queue. These threads will make use of the producer consumer model to communicate. The producer will be the dispatcher thread while the worker thread will be the consuemr. Below is the psuedo code of how they commincate.

```
producer(task):
    lock(mutex)
    if queue is full:
        wait(mutex)
    enqueue(task)
    signal(consumer)
    unlock(mutex)

consumwr():
    lock(mutex)
    if queue is empty:
        wait(mutex)
    ret_val = dequeue()
    signal(producer)
    unlock(mutex)
    return ret_val
```

## Atomicity

In this assignment we make use of a function flock to lock and unsure read write atomicity. Lock makes use of different types of locks:

- Shared Locks - Multiple workers can hold this lock on the same file (used for GET)
- Exclusive Locks - Only one worker can hold this for one file (used for PUT and APPEND)

Further I log in the file during these locks to unsure that the logs reprsents what actually happens at that time.

Lastly I read in the message body all at once before I open the file. I do this to ensure that a GET request that was sent during a PUT request reflects the previous state of the file rather than hanging and waiting for the PUT to finish.
I also now truncate the file in PUT requests after I open the file to ensure that any modifications done to the file are done inside the flocks.

## Files

httpserver.c - The HTTP server, holds main()

io.c - Contains read, write, and open wrappers around read(2), write(2), and open(2)

parser.c - Contains functions to parser an incoming request from a client

process.c - Contains functions to process a parsed request

talk.c - Contains functions used to send and receive requests/responses to/from the client

Makefile - Used to build the server

## Build

    $ make

    $ make all

    $ make httpserver

## Running / Usage

    $ ./httpserver <socket>

## Cleaning

    $ make clean
