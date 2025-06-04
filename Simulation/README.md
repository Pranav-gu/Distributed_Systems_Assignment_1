
# Balls and Grid Simulation

This project simulates the movement and collision of balls on a 2D toroidal grid using MPI for parallel processing. The balls move in given directions (up, down, left, right), and interact (bounce back or reflect) based on the number of collisions at each cell.

## Table of Contents

- [Overview](#overview)
- [How it Works](#how-it-works)
- [Build and Run Instructions](#build-and-run-instructions)
- [Input Format](#input-format)
- [Output Format](#output-format)
- [Dependencies](#dependencies)

## Overview

This program performs a distributed simulation of balls moving across an `n x m` toroidal grid. Each ball has a direction and position, and moves at each time step. MPI is used to parallelize the simulation across multiple processes by partitioning rows.

Key Features:
- Directional movement (U, D, L, R)
- Wraparound (toroidal) behavior on edges
- Collision handling (2 or 4 ball collisions)
- Efficient MPI communication and custom datatype usage

## How it Works

1. **Initialization (Rank 0 only):**
   - Reads grid dimensions `n`, `m`, number of balls `k`, and number of time steps `t`.
   - Initializes ball positions and directions.

2. **MPI Broadcast & Process Grouping:**
   - Broadcasts grid parameters to all ranks.
   - Excludes unnecessary ranks if more processes than rows.

3. **Data Distribution:**
   - Distributes balls to responsible processes based on their initial row.

4. **Simulation Loop:**
   - Each time step involves:
     - Moving balls.
     - Handling wraparound and inter-process communication.
     - Processing collisions:
       - 2-way: directions reversed clockwise.
       - 4-way: directions reversed directly (U↔D, L↔R).

5. **Result Collection (Rank 0 only):**
   - Gathers final ball positions and prints them in order of input.

## Build and Run Instructions

### Compile

```bash
mpic++ -std=c++17 -O2 balls_simulation.cpp -o balls_simulation
```

### Run

```bash
mpirun -np <num_processes> ./balls_simulation
```

Replace `<num_processes>` with the number of processes to run (ideally ≤ number of rows).

## Input Format

Provided through standard input (e.g., via file redirection or manual entry).

```
n m k t
x1 y1 d1
x2 y2 d2
...
xk yk dk
```

- `n` `m`: Grid size
- `k`: Number of balls
- `t`: Number of time steps
- Each of the next `k` lines has:
  - `xi`, `yi`: 0-indexed position of ball `i`
  - `di`: Direction (`U`, `D`, `L`, `R`)

## Output Format

After `t` time steps, final positions and directions of balls are printed, one per line, in input order:

```
x y d
...
```

## Dependencies

- MPI (OpenMPI or MPICH)
- C++17 or later

## License

This simulation is released for educational and research purposes.
