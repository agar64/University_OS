# **Multithreaded Client Processing System**  

## **Project Overview**  
This project is a **multithreaded and multiprocess client processing system** implemented in **C on Linux** using **pthreads and semaphores**. The goal is to simulate a reception and service system where clients arrive, wait in a queue, and are attended by a service thread. Additionally, a separate **analyst process** reads and processes completed service requests.  

## **How It Works**  

- A **Reception Thread** generates clients and adds them to a queue. Each client is assigned a random **priority** (high or low).  
- A **Service Thread** removes clients from the queue and processes them. The time a client is willing to wait depends on their priority.  
- A **File-Based Communication System** stores the IDs of attended clients in `LNG.txt`.  
- An **Analyst Process** reads up to **10 client IDs** at a time from `LNG.txt`, prints them, and removes them from the file.  

## **Client Prioritization**  
- **High-priority clients** have a maximum waiting time of **X/2 milliseconds**.  
- **Low-priority clients** have a maximum waiting time of **X milliseconds**.  
- A client is **satisfied** if they are served within their patience limit.  

## **Program Termination Conditions**  
- If a fixed number of clients (`N`) is set, the program stops once all clients have been processed.  
- If `N = 0`, the reception creates an **infinite number of clients**, and the program stops only when the **"s" key is pressed**.  

## **Compilation & Execution**  

### **Compiling the Code**  
To compile the program, run:  
```bash
gcc file.c -o file -lpthread
```
Where file is the name of the file you're compiling, for each of the three files.

### **Running the Program**  
```bash
./atendente <N> <X>
```
Where:  
- `N`: Number of clients to be generated (use `0` for infinite clients).  
- `X`: Maximum patience time (ms) for low-priority clients (high-priority = `X/2`).  

Example:  
```bash
./atendente 10 500
```
Creates **10 clients**, with low-priority clients having **500ms patience** and high-priority clients **250ms patience**.  

To stop the program in infinite mode, press **"s"**, and then **"Enter"**. 
