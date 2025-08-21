## MiniDB
Welcome to the MiniDB SQL Database project! Once completely built it will be a fully functional, disk-based relational database management system (RDBMS) written from scratch in modern C++. The primary goal of this project is to serve as a learning tool for understanding the internal workings of a database, from SQL parsing to transaction management, data storage and indexing on disk.

## Features

- Custom SQL Parser: A handwritten recursive descent parser that builds a detailed Abstract Syntax Tree (AST) from raw SQL queries.

- ACID Compliance: Designed to ensure Atomicity, Consistency, Isolation, and Durability for all transactions.

- Disk-Based Storage: A page-oriented storage engine that persists data to disk.

- B+ Tree Indexing: For efficient data retrieval and query optimization.

- Client-Server Architecture: A simple TCP-based server and client library for remote connections.

- Command-Line Interface (CLI): A user-friendly tool for interacting with the database.



## Prerequisites
- A C++17 compliant compiler (e.g., GCC, Clang, MSVC)

- CMake (version 3.14 or higher)
  
- Google Test (will be downloaded automatically by CMake)

- Doxygen (optional, for generating documentation)

## Building the Project
- Clone the repository:
```
git clone https://github.com/amitvc/minidb.git
cd minidb
```

- Configure with CMake:
```
TODO
```

- Compile the code:
```
TODO
```

  
