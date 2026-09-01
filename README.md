# miniserv-Exam-Rank-06

# 42 Exam Rank 06

This repository contains my solution for **Exam Rank 06 at 42 Istanbul**, completed successfully on **August 21, 2026**.

The exam focuses on **C system programming and network programming**, testing problem-solving skills under time constraints.

During the exam, I implemented a **multi-client TCP server** using `select()` for I/O multiplexing. The server handles client connections, message reception, message broadcasting, disconnections, and buffered/partial writes.

## 📂 Repository Structure

```text
exam06-main/

└── mini_serv/
    └── mini_serv.c
```

## 🧠 Key Concepts

* TCP Socket Programming
* Client/Server Architecture
* `socket()`, `bind()`, `listen()`, `accept()`
* `send()` / `recv()`
* I/O Multiplexing with `select()`
* File Descriptor Management
* Client Input/Output Buffers
* Partial Reads and Writes
* Dynamic Memory Management
* Error Handling

## 🎯 Exam Solution

The solution manages multiple clients within a single process using `select()`, without threads or processes.

Incoming data is buffered and separated into messages using `\n`, while outgoing data is handled through per-client buffers to account for partial `send()` operations.
