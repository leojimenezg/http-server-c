# HTTP Server C

A lightweight HTTP server built from scratch in C to practice low-level programming and understand web server fundamentals.

---

## Installation

To get this project up and runnig on your local machine, follow the next instructions.

### Prerequisites
Before anything else, make sure you have installed **C23** (or newer) and a compatible compiler like **GCC** or **Clang** in your system. 

**Note**: This server uses POSIX threads (`pthread`) and socket APIs, so it's designed for Unix-like systems.

### Steps
1. **Clone the repository:**
Open your prefered terminal and clone the project to your local machine.
    ```bash
    git clone https://github.com/leojimenezg/http-server-c.git
    ```
2. **Navigate into the project directory:**
    ```bash
    cd http-server-c
    ```
3. **Build the project**
Choose the compilation method that applies to your setup.
    * If you compile using GCC:
        ```bash
        gcc -std=c23 src/server.c -o src/server -lpthread
        ```
    * If you compile using Clang:
        ```bash
        clang -std=c23 src/server.c -o src/server -lpthread
        ```
4. **Run the application**
Finally, execute the generated object file to launch the Http Server C Program.
    ```bash
    ./src/server <IP_ADDRESS> <PORT> <MAX_CONNECTIONS>
    ```
    **Example**:
    ```bash
    ./src/server 127.0.0.1 8080 10
    ```

---

## How Does It  Work?

This HTTP server is a simple project specifically made to learn C programming. However, it has some simple but powerful capabilities.

### Key Features
* **Static content serving**: Handles only static files (HTML, CSS, JavaScript).
* **Whitelisted resources**: Only serves predefined resources for security.
* **Multithreading**: Uses POSIX threads to handle multiple client connections simultaneously.
* **HTTP/1.1 compliance**: Implements basic HTTP/1.1 request/response handling.

### Available Routes
The server currently supports these routes:
* `/` - `/index`: Main page.
* `/about`: About page.
* `/styles`: CSS stylesheet.
* `/script`: JavaScript file.
* `/favicon.ico`: Website icon.

### Request Processing
When the server receives a request, it processes it through these steps:
1. **HTTP verb validation**: Only accepts `GET` requests.
2. **URL extraction and validation**: Parses the URL from the request line.
3. **Resource lookup**: Matches the URL against the whitelist.
4. **File serving**: Reads and serves the requested file with appropriate headers.

### Error Handling
The server responds with appropriate HTTP status codes:
* **200 Ok**: Successful request with file content.
* **400 Bad Request**: Malformed request or unsupported HTTP verb.
* **404 Not Found**: Requested resource not in whitelist or file not found.
If internal server errors occur after accepting a connection, the server closes the connection and performs cleanup.

### Technical Details
* Only processes the first line of HTTP requests (method, URL, version).
* Ignores HTTP headers sent by clients.
* Uses atomic operations for connection counting.
* Implements proper resource cleanup and memory management.

---

## Notes

* This project was created as a learning exercise for C programming. This is my first substantial project in C, so while it may not be production-ready, it was an excellent learning experience.
* The server is designed for educational purposes and local development only. It lacks many security features required for production use.
* Currently, I don't plan to add extra capabilities, but I'm open to using this as a foundation for future projects or improvements.
* The server has been tested only for its intended purposes. Behavior outside these scenarios is not guaranteed.
* Feel free to suggest changes, fork the project, or extend its functionality for your own needs.

---

## Useful Resources

* [C Reference Documentation](https://cppreference.com/w/c.html)
* [POSIX Threads Programming](https://computing.llnl.gov/tutorials/pthreads/)
