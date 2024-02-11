# Metro Network Monitor

## About the Project
Metro Network Monitor is a software solution aimed at optimizing passenger flow across metro systems by suggesting less crowded routes to commuters. This project leverages modern C++ features, including automatic memory management, asynchronous operations, lambdas, templates and more, along with libraries such as Boost.Asio and JSON for Modern C++. The network monitor operates a STOMP server that offers a quiet route service to any STOMP clients making itinerary requests.

### Design Considerations

- **Objective**: Develop an itinerary recommendation system that monitors and analyzes real-time metro network traffic to suggest quieter routes.
- **Data Sources**: Utilizes real-time data from electronic card taps and a JSON-based network layout.
- **Technical Aspects**: Functions as both client and server, using STOMP over WebSockets for messaging, and stores data in JSON format.

### Prerequisites

- GCC compiler
- CMake and Make for build generation
- Conan package manager
- Git for version control

### Installation

1. **Clone the repository**

```bash
git clone <repository-url>
```
2. **Create cmake build directory**
```bash
mkdir build && cd build
```
3. **Install dependencies with Conan**
```bash
pip3 install conan==1.53.0
conan install .. --build=missing
```
4. **Configure and build the project with CMake**
```bash
cmake ..
cmake --build . --parallel $(nproc)
```
