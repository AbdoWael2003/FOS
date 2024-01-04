# FOS: Simple Operating System

## Overview
FOS (FCIS Operating System) is a simple operating system developed in C language, with a focus on memory management and a BSD-inspired CPU scheduler. This project aims to provide a foundation for understanding the fundamental aspects of operating system development, making it suitable for educational purposes.

## Features
1. **Memory Management:**
   - Page Allocator: Efficiently manages physical memory using page-based allocation.
   - Dynamic Allocator: Allocates memory for sizes less than 2 kilobytes dynamically.

2. **CPU Scheduler:**
   - BSD Scheduler: Implements a simplified version of the BSD scheduler for process scheduling.
   


## Getting Started
1. **Clone the Repository:**
   ```bash
   git clone https://github.com/your-username/fos.git
   ```

2. **Build the Operating System:**
   ```bash
   cd fos
   make
   ```


## Memory Management

### Page Allocator
The page allocator manages physical memory by dividing it into pages and allocating them to the operating system components as needed. This ensures efficient use of memory resources.

### Dynamic Allocator
The dynamic allocator handles dynamic memory allocation for sizes less than 2 kilobytes. It provides a flexible memory allocation mechanism to support variable-sized data structures.

## CPU Scheduler

### BSD Scheduler
The BSD scheduler is a process scheduler that assigns a priority to each process and allocates CPU time based on these priorities. It ensures fairness and responsiveness in handling various tasks.

## Contribution Guidelines
We welcome contributions and improvements to FOS. If you have any suggestions, bug fixes, or new features to add, please follow these steps:
1. Fork the repository.
2. Create a new branch for your changes: `git checkout -b feature-name`.
3. Make your changes and commit them: `git commit -m "Description of changes"`.
4. Push to your branch: `git push origin feature-name`.
5. Create a pull request.

## Acknowledgments
- This project is inspired by the desire to learn and share knowledge about operating system development.
- Thanks for DR - Ahmed Salah for his support allong the project

## Development Team 
- Ahmed Sameh Ahmed Abdelaziz
- Abdelrahman Wael
- Rawan Mohamed
- Jannah Mahmoud
- Nour Ahmed
- Sara Ahmed 

Feel free to explore and enhance FOS as you delve into the fascinating world of operating system development!
