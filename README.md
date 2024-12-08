# Traffic Simulator Project 
A multithreaded traffic simulator implemented in C that simulates vehicles moving on a road network with traffic lights.
## Description
This project implements a traffic simulator with two modes:

Basic Mode: Sequential vehicle movement
Concurrent Mode: Multithreaded vehicles and traffic lights

## Prerequisites
 **GCC compiler**
- **POSIX threads library** (pthread)
- **Linux/Unix** environment

## Installation
1. **Clone the repository :**

   ```bash
   git clone https://github.com/YassineM0/C-multithreading-traffic-simulation.git
   cd C-multithreading-traffic-simulation

2. **Compiling :**

   ```bash
   gcc -o Main Main.c -pthread

## Configuration
Create nombres.txt with the format:
Copyheight width
number_of_vehicles
horizontal_roads vertical_roads
**Example**:
Copy10 30
10 30 
5 
1 1 
 **Usage**
 

3. **Run the simulator: :**

   ```bash
   ./Main

4. **Features :**
🎯 

Visual Representation:

Vehicles: *
Roads: -, |
Traffic Lights: R, V


Thread-safe operations
Traffic light synchronization
Collision avoidance system

## Implementation Details
Key Structures
cCopytypedef struct {
    int x;
    int y;
    char etat;
    char direction;
    pthread_mutex_t mutex;
} FeuSignalisation;

typedef struct {
    char **grille;
    int hauteur;
    int largeur;
    int x;
    int y;
    char direction;
    pthread_mutex_t *mutex_grille;
    PaireFeux **paires_feux;
    int nb_paires;
} ThreadParams;
