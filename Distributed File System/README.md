# Distributed File System (DFS)

This document describes a simple distributed file system implemented using MPI for inter-process communication. The system allows users to upload, retrieve, search, and list files, while also incorporating a basic heartbeat mechanism for fault tolerance.

## Overview of Operations

This Distributed File System (DFS) operates with a metadata server (rank 0) and multiple storage nodes (ranks 1 to size-1).

**Key functionalities include:**

- **Upload:** Divides files into 32-byte chunks and replicates them across three available storage nodes. The metadata server keeps track of which chunks are stored on which nodes.
- **Retrieve:** Reconstructs a file by retrieving its chunks from any available storage node that holds them.
- **Search:** Searches for a specific word within a file and returns the starting positions of the word.
- **List File:** Displays the chunk distribution for a given file, showing which nodes hold each chunk and how many healthy replicas exist.
- **Heartbeat Mechanism:** Storage nodes periodically send heartbeats to the metadata server. If a storage node misses heartbeats for a certain duration, it's marked as "failed" by the metadata server.
- **Failover:** Manually simulates the failure of a storage node.
- **Recover:** Manually simulates the recovery of a failed storage node.
- **Available:** Lists all currently healthy storage nodes.

## Requirements

To compile and run this code, you will need:

- **MPI Library:** A Message Passing Interface (MPI) implementation (e.g., OpenMPI, MPICH).
- **C++ Compiler:** A C++ compiler that supports C++11 or newer (for `std::thread`, `std::atomic`, `std::chrono`, `std::mutex`).
- **Standard C++ Libraries:** `iostream`, `fstream`, `string`, `vector`, `map`, `set`, `sstream`, `algorithm`, `thread`, `atomic`, `chrono`, `mutex`.

## Execution Explanation

### Compilation

You'll typically compile this code using an MPI wrapper for your C++ compiler. For example, with OpenMPI:

```sh
mpic++ -o dfs_server your_file_name.cpp -std=c++11
```

Replace `your_file_name.cpp` with the actual name of your source file (e.g., `dfs.cpp`).

### Running the System

To run the distributed file system, you need to launch it with `mpirun` (or `mpiexec`), specifying the number of processes (`-np`). The first process (rank 0) will act as the metadata server, and the subsequent processes will be storage nodes.

```sh
mpirun -np <number_of_processes> ./dfs_server
```

**Example:** To run with 5 processes (1 metadata server, 4 storage nodes):

```sh
mpirun -np 5 ./dfs_server
```

## Interacting with the System (Metadata Server - Rank 0)

Once the system is running, you can interact with it by typing commands into the standard input of the metadata server (the terminal where you executed `mpirun`).

### Available Commands

- **upload `<filename>` `<filepath>`**: Uploads a file.
    - `<filename>`: The name you want to assign to the file within the DFS.
    - `<filepath>`: The actual path to the file on the local machine where the dfs_server is run.
    - *Example:* `upload mydocument.txt /home/user/documents/report.txt`

- **retrieve `<filename>`**: Retrieves a file.
    - `<filename>`: The name of the file in the DFS to retrieve.
    - *Example:* `retrieve mydocument.txt` (The content will be printed to the console.)

- **search `<filename>` `<word_to_search>`**: Searches for occurrences of a word in a file.
    - `<filename>`: The name of the file in the DFS.
    - `<word_to_search>`: The word you want to find.
    - *Example:* `search mydocument.txt important`

- **list_file `<filename>`**: Lists the chunk distribution for a file.
    - `<filename>`: The name of the file in the DFS.
    - *Example:* `list_file mydocument.txt`

- **failover `<node_id>`**: Simulates a failure of a specific storage node.
    - `<node_id>`: The rank of the storage node to fail (e.g., 1, 2).
    - *Example:* `failover 1`

- **recover `<node_id>`**: Simulates the recovery of a specific storage node.
    - `<node_id>`: The rank of the storage node to recover.
    - *Example:* `recover 1`

- **available**: Lists the IDs of all currently available (healthy) storage nodes.
    - *Example:* `available`

- **exit**: Shuts down the DFS.
