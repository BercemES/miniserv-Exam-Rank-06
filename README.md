42 Exam Rank 06

This repository contains my solution for Exam Rank 06 at 42 Istanbul, completed successfully on August 21, 2026.

The exam focuses on C system programming and network programming, with a multi-client TCP server implemented under a predefined set of allowed functions and constraints.

Under these conditions, the solution handles multiple clients using select() for I/O multiplexing, including client connections, message reception, broadcasting, disconnections, and buffered writes.

📂 Repository Structure
exam06-main/

└── mini_serv/
    └── mini_serv.c
🧠 Key Concepts
TCP Socket Programming
Client/Server Architecture
socket(), bind(), listen(), accept()
send() / recv()
I/O Multiplexing with select()
File Descriptor Management
Client Buffers
Partial Reads and Writes
Dynamic Memory Management
Error Handling
🎯 Solution

The server manages multiple clients within a single process using select(), without threads or processes.

Incoming data is buffered and separated into messages using \n, while outgoing data is handled through per-client buffers to account for partial send() operations.

The extract_message and str_join helper functions were provided as part of the exam template.
