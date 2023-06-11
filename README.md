- [Easy-gRPC](#easy-grpc)
  - [Features](#features)
  - [Installation](#installation)
  - [Examples](#examples)
    - [Proto/RPC code auto-generation](#protorpc-code-auto-generation)
    - [Simple unary call](#simple-unary-call)

# Easy-gRPC

## Features

- Automatic code generation for *.proto files.
- Asynchronous call with only one line of code.
- Asynchronous callback support.
- Arbitrary RPC types support.

## How to use

- Build and install the gRPC library, see the [How to use gRPC](https://github.com/grpc/grpc/tree/master/src/cpp#to-start-using-grpc-c)
  instructions for guidance on how to add gRPC as a dependency to current project. Not that you have to install the gRPC
  library as CMake package to make use of it in current project.
- After installation of the gRPC library, install current project as CMake package:
  - Linux/Unix
    ```shell
    $ mkdir -p build
    $ cd build
    $ cmake -DCMAKE_PREFIX_PATH="your/grpc/installation/directory" -DCMAKE_INSTALL_PREFIX="your/install/destination" ..
    $ make install
    ```
  - Windows, from Visual Studio command prompt
    ```bat
    > md build
    > cd build
    > cmake -G "NMake Makefiles" -DCMAKE_PREFIX_PATH="your/grpc/installation/directory" -DCMAKE_INSTALL_PREFIX="your/install/destination" ..
    > nmake install
    ```
- To use the installed package in your CMake project, you have to add the gRPC library and current project installation
  path to CMake prefix path ``CMAKE_PREFIX_PATH`` either by adding cmake options ``-DCMAKE_PREFIX_PATH`` when generating
  the project build system or append the path by cmake code (``set(CMAKE_PREFIX_PATH ...)`` or
  ``list(APPEND CMAKE_PREFIX_PATH ...)``).
- Now you are ready to make use of the project dependency package in CMake canonical way:
  ```cmake
  find_package(EasyGRPC CONFIG REQUIRED)
  add_executable(YourExe YourCode.cpp)
  target_link_libraries(YourExe ShuHai::gRPC)
  ```

## Examples

### Proto/RPC code auto-generation

To make use of gRPC services or Protobuf messages, a proto file is required to define the RPC services and protocol
messages, generate the corresponding RPC services and messages code through the protoc executable and the corresponding
gRPC plug-in, and use the generated code to implement custom logic.  
Doing the work described above manually is tedious and error-prone. The library provides a CMake function to
automatically complete the work.

Here's an example of typical usage of the CMake function:

```cmake
shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET HelloWorld-Proto-Gen
        LIBRARY_TARGET HelloWorld-Proto
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/HelloWorld.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")
```

The CMake function ``shuhai_grpc_add_proto_targets`` creates 2 targets:

- The target specified by the option ``GENERATOR_TARGET`` is used to generate proto code define by proto files which
  specified by ``PROTO_FILES`` option, the proto code is automatically generated whenever the specified proto file
  changed.
- The target specified by the option ``LIBRARY_TARGET`` is used to build the generated code as a static library, the
  library target depends on the code generator target above, i.e. whenever the code generator target regenerates the
  code, the library target rebuilds.

The users' targets can now links the library target to make use of the generated code:

```cmake
# "HelloWorld-Server" is the user target serve as gRPC server.
target_link_libraries(HelloWorld-Server
        PRIVATE HelloWorld-Proto)
add_dependencies(HelloWorld-Server HelloWorld-Proto)

# "HelloWorld-Client" is the user target serve as gRPC client.
target_link_libraries(HelloWorld-Client
        PRIVATE HelloWorld-Proto)
add_dependencies(HelloWorld-Client HelloWorld-Proto)
```

Now it is guaranteed that the latest proto file and its corresponding code are compiled every time the user targets
``HelloWorld-Server`` or ``HelloWorld-Client`` builds.

In addition to the above usage of the CMake function, you can also add the generated code to your existing target so
that the generated code is built as part of your target:

```cmake
shuhai_grpc_add_proto_targets(
        GENERATOR_TARGET HelloWorld-Proto-Gen
        LIBRARY_TARGET HelloWorld # "HelloWorld" is the name of a user target.
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/HelloWorld.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")
```

### Simple unary call

Here's an example shows how to build a server to handle unary calls, and build a client to perform unary calls.  
We define a proto service and related messages as follows:

```protobuf
syntax = "proto3";

package HelloWorld;

service Greeter
{
    rpc SayHello(HelloRequest) returns(HelloReply) {}
}

message HelloRequest
{
    string name = 1;
}

message HelloReply
{
    string message = 1;
}
```

Instantiate the server class template with the above service to handle RPCs defined by the service and build the server
object with listening URLs or port:

```c++
using RpcServer = ShuHai::gRPC::Server::AsyncServer<HelloWorld::Greeter::AsyncService>;
RpcServer server(12345);
```

Define a function using request and response types as parameters to handle custom business logic, and register the
function to the above server object. Note that:

- The RPC handler is registered with the generated function which in form of 'AsyncService::Request\<ServiceMethodName\>`.
- The handler function is called in thread other than your current thread, you should take care of the thread safety.

```c++
static HelloReply SayHello(grpc::ServerContext& context, const HelloRequest& request)
{
    HelloReply reply;
    // Implement business logic here.
    return reply
}
server.registerCallHandler(&Service::RequestSayHello, &SayHello);
```

Now start the server:

```c++
server.start();
```

Next, let's see how to build the client and perform a certain RPC.  
We define the client type for certain services by instantiate the client class template just like how we define the
corresponding server class above:

```c++
using RpcClient = ShuHai::gRPC::Client::AsyncClient<HelloWorld::Greeter::Stub>;
RpcClient client("localhost:12345");
```

Declare the RPC parameter (request) variable and populate contents according to your needs:

```c++
HelloRequest request;
request.set_name("user");
```

Now we can perform the asynchronous call by a simple line of code:

```c++
std::future<HelloReply> f = client.call(&Greeter::Stub::PrepareAsyncSayHello, request)->response();
// Wait for the result ready and get the result.
auto reply = f.get();
printf("SayHello reply: %s", reply.message().c_str());
```

Note that there are two ways to get the result, the one is demonstrated above, the other is to get the result by a
callback function:

```c++
client.call(&Greeter::Stub::AsyncSayHello, request,
    [](std::future<HelloReply>&& f)
    {
        try
        {
            auto reply = f.get();
            printf("SayHello reply: %s", reply.message().c_str());
        }
        catch (const ShuHai::gRPC::Client::AsyncCallError& e)
        {
            printf("Handler failed(%d): %s", e.status().error_code(), e.what());
        }
        catch (const std::exception& e)
        {
            printf("Handler failed: %s", e.what());
        }
    });
```
