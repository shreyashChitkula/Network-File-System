## Assumptions
#### Storage Server
- every storage server has it's own folder and if you want to access data within that server you will have to use that folder
#### Naming Server
- for efficient searching using Tries
- LRU caching using doubly linked list

# Network File System (NFS) - Project README

## Project Overview

This project implements a simple yet robust Network File System (NFS) from scratch using the C programming language. The primary goal is to enable clients to seamlessly interact with remote file systems using functionalities like reading, writing, deleting, and streaming files. The system consists of three core components:

1. **Clients**: Interfaces for users to interact with the NFS.
2. **Naming Server (NM)**: A central coordination hub managing directory structures and file locations.
3. **Storage Servers (SS)**: Responsible for data storage and retrieval, handling client requests, and maintaining data consistency.

## Design Approach

The system is designed to be modular and scalable, adhering to the following principles:

- **Decoupled Components**: The Naming Server, Storage Servers, and Clients operate independently, communicating via TCP sockets.
- **Dynamic Scaling**: New storage servers can be added dynamically.
- **Fault Tolerance**: Data is replicated across multiple servers to ensure availability.
- **Efficiency**: Employing caching and optimized search algorithms for quick responses.

## Setup and Installation

### Prerequisites

- GCC Compiler
- Linux/Unix-based environment (recommended)
- Basic knowledge of Makefiles

### Installation

1. Clone the repository:
   ```bash
   git clone <repository_url>
   cd <repository_folder>
   ```

2. Compile the project using the provided Makefile:
   ```bash
   make all
   ```

3. Start the Naming Server:
   ```bash
   ./a.out 
   ```

4. Initialize Storage Servers:
   ```bash
   ./a.out <storage:server:ip:address>
   ```

5. Start the Client:
   ```bash
   ./client
   ```

> NOTE: make changes in headers for ip address to ensure propper working 


## Features

### Core Functionalities

1. **File Operations**:
   - Create, read, write, and delete files or folders.
     - Example: Create a file and write data into it using the `CREATE` and `WRITE` commands.
   - Synchronous and asynchronous file operations for efficiency.

2. **Asynchronous and Synchronous Writing**:
   - **Asynchronous Writing**: Allows clients to send large file data without waiting for the entire write operation to complete. Data is written to local memory and periodically flushed to persistent storage in fixed-size chunks.
   - **Acknowledgment Mechanism**: Immediate acknowledgment is sent to the client upon receiving data. A final acknowledgment is sent after successful write completion.
   - **Priority Writes**: Clients can prioritize synchronous writes using a `--SYNC` flag or equivalent.
   - **Failure Handling**: If a Storage Server fails during an asynchronous write, the Naming Server notifies the client.

3. **Multiple Clients**:
   - **Concurrent Client Access**: Supports multiple clients interacting with the Naming Server simultaneously. Initial acknowledgment ensures non-blocking operations.
   - **Concurrent File Reading**: Multiple clients can read the same file simultaneously, but only one client can write to a file at a time. Other clients are restricted until the write completes.

4. **Metadata Retrieval**:
   - Obtain details like file size, permissions, and timestamps to support informed client interactions.

5. **Audio Streaming**:
   - Stream audio files in real-time directly from the server.
     - Example: Play music files on the client-side using the `STREAM` command.

6. **Dynamic Scaling**:
   - Add or remove storage servers dynamically without disrupting the system.

7. **Caching and Optimization**:
   - Efficient search mechanisms in the Naming Server using Tries or Hashmaps for quick lookups.
   - LRU (Least Recently Used) caching for recent searches to improve performance.

8. **Replication and Fault Tolerance**:
   - Data replication across multiple Storage Servers ensures fault tolerance and availability.
   - Asynchronous duplication of write operations to replicated stores for performance.

9. **Error Codes**:
   - Defined error codes for scenarios like unavailable files, permission issues, and access conflicts.

10. **Backing up Data**:
    - **Failure Detection**: Detects Storage Server failures to ensure uninterrupted service.
    - **Data Replication**: Maintains multiple copies of files across Storage Servers for redundancy. Read operations are supported during failures.
    - **SS Recovery**: Matches replicated stores with the original server upon recovery.

11. **Redundancy**:
    - Ensures no data loss when a Storage Server reconnects to the Naming Server.

12. **Bookkeeping**:
    - Logs every request and acknowledgment with timestamps, IPs, and ports.
    - Displays real-time status messages for ongoing operations.

### Example Commands

- **Read a file**:
  ```bash
  READ /dir1/dir2/file.txt
  ```
- **Create a file**:
  ```bash
  CREATE /dir1/dir2/new_file.txt
  ```
- **Stream an audio file**:
  ```bash
  STREAM /music/song.mp3
  ```
- **Copy a file**:
  ```bash
  COPY /source_path /destination_path
  ```

### Command Syntax

- Commands are parsed by the Naming Server, which identifies the appropriate Storage Server for execution.

## Codes Used in the Project

| Error Code | Description                          |
|------------|--------------------------------------|
| 1-8        | Requests(List,Read,Write,Create,Delete,Copy,Info,Stream)|
| 9          | Error                      |
| 10        | Success                       |
| 11        | Stop     |
| 12        | Response               |

## Technical Details

- **Programming Language**: C
- **Protocol**: TCP sockets ensure reliable communication between components.
- **Data Structures**: Custom hashmaps and linked lists are used for efficient storage and lookups.
- **Asynchronous Writing**: Buffered writes with periodic flushing to disk for performance.
- **Replication**: Files are replicated on at least two additional servers (if available) for fault tolerance.

## Future Improvements

1. Enhance fault tolerance by introducing quorum-based consistency.
2. Implement secure file transfer using encryption.
3. Add a graphical user interface for user-friendly interactions.
4. Extend support for larger datasets and concurrent clients.

---

Feel free to reach out for any queries or contributions!

## Team
1. [Ch.Shreyash](https://github.com/shreyashChitkula)
2. [Maryala Sai Teja](https://github.com/saitejaMaryala)
3. [Prajas Wadekar](https://github.com/PrajasW)
4. [Vishnu Sathwik reddy]()



