
# Parallel Reverse BFS using MPI

## Overview
This MPI-based C++ program performs a **parallel reverse breadth-first search (BFS)** to compute the minimum number of steps required for each explorer node to reach a designated *exit node* in a directed or undirected graph. The computation is distributed across multiple MPI processes.

## Functionality
- Construct a graph from a given input file.
- Remove edges connected to specified “blocked” nodes.
- Use reverse BFS starting from the exit node to compute shortest paths.
- Parallelize the BFS computation using MPI.
- Output the distance (in steps) from each explorer node to the exit node.

## Requirements
- MPI environment (e.g., `OpenMPI` or `MPICH`)
- C++ compiler with MPI support (e.g., `mpic++`)
- Input file in specified format

## Input File Format
The program expects an input file as the first command-line argument, structured as follows:

```
n m                           // Number of nodes and edges
u1 v1 d1                      // Edge from u1 to v1, d1 = 1 if undirected, else directed; repeated m times
k                             // Number of explorers
e1 e2 e3 ... ek               // Explorer node IDs
exit_node                     // The designated exit node
num_blocked                   // Number of blocked nodes
b1 b2 ... bN                  // List of blocked node IDs
```

## Execution Command
Compile and run the program using:

```bash
mpic++ -o reverse_bfs Parallel_BFS.cpp
mpirun -np <num_processes> ./reverse_bfs input.txt output.txt
```

## Execution Flow

1. **MPI Initialization:**  
   The program initializes MPI and retrieves the total number of processes and the rank of each.
   
2. **Input Parsing (Rank 0):**
   - Reads graph structure and properties.
   - Builds an adjacency list.
   - Builds the reverse adjacency list for BFS.
   - Removes all edges involving blocked nodes.

3. **Broadcast:**  
   The reverse adjacency list, explorer data, and other metadata are broadcast to all processes.

4. **Reverse BFS:**
   - Starts from the *exit node*.
   - At each level, frontier nodes are processed in parallel to identify their predecessors.
   - Nodes are assigned to processes based on `node_id % size`.
   - Inter-process communication is handled via `MPI_Sendrecv` to exchange frontier data.

5. **Termination:**  
   When no new nodes are discovered across all ranks, the BFS ends.

6. **Output:**
   - Only the process responsible for the exit node (`exit_node % size`) writes the distances of explorer nodes to the output file.

## Output
The output file (second command-line argument) contains space-separated integers, each representing the minimum number of steps required for an explorer to reach the exit node.
