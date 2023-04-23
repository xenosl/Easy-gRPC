- [gRPC-Quick](#grpc-quick)
    - [Design Goals](#design-goals)
    - [Examples](#examples)
        - [Proto/RPC code auto-generation](#protorpc-code-auto-generation)
        - [Simple unary call](#simple-unary-call)

# gRPC-Quick

## Design Goals

- **Automatic code generation**. gRPC uses protobuf with a custom plugin to generates codes for serialization and rpc
  methods. It is tedious and error-prone to do it manually, the project provides convenient CMake functions to achieve
  it automatically.
- **Simplify asynchronous call & handling**. Simplify the use of asynchronous call/handling with arbitrary RPC types
  with all types of RPC methods, i.e. unary call, client/server streaming, bidirectional streaming. (But streaming is
  not implemented yet for now)

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
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/helloworld.proto"
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
        PROTO_FILES "${CMAKE_CURRENT_LIST_DIR}/proto/helloworld.proto"
        OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/src/Generated")
```

### Simple unary call

Here's an example shows how to build a server to handle unary calls, and build a client to perform unary calls.  
We define a proto service and related messages as follows:

```protobuf
syntax = "proto3";

package helloworld;

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
using RpcServer = ShuHai::gRPC::Server::AsyncServer<helloworld::Greeter::AsyncService>;
RpcServer server(12345);
```

Define a function using request and response types as parameters to handle custom business logic, and register the
function to the above server object. Note that:

- The RPC handler is registered with the generated function which in form of 'AsyncService::Request\<ServiceMethodName\>`.
- The handler function is called in thread other than your current thread, you should take care of the thread safety.

```c++
static void StartTask(const Proto::Task::StartTaskRequest& request, Proto::Task::StartTaskReply& reply)
{
    // Implement business logic here.
}
server.registerCallHandler(&Service::RequestStartTask, &StartTask);
```

Now start the server:

```c++
server.start();
```

Next, let's see how to build the client and perform a certain RPC.  
We define the client type for certain services by instantiate the client class template just like how we define the
corresponding server class above:

```c++
using RpcClient = ShuHai::gRPC::Client::AsyncClient<helloworld::Greeter::Stub>;
RpcClient client("localhost:12345");
```

Declare the RPC parameter (request) variable and populate contents according to your needs:

```c++
HelloRequest request;
request.set_name("user");
```

Now we can perform the asynchronous call by a simple line of code:

```c++
std::future<HelloReply> f = client.call(&Greeter::Stub::PrepareAsyncSayHello, request);
// Wait for the result ready and get the result.
auto reply = f.get();
printf("SayHello reply: %s", reply.message().c_str());
```

Note that there are two ways to get the result, the one is demonstrated above, the other is to get the result by a
callback function:

```c++
client.call(&Greeter::Stub::PrepareAsyncSayHello, request,
    [](std::future<HelloReply>&& f)
    {
        try
        {
            auto reply = f.get();
            printf("SayHello reply: %s", reply.message().c_str());
        }
        catch (const ShuHai::gRPC::Client::AsyncCallError& e)
        {
            printf("Call failed(%d): %s", e.status().error_code(), e.what());
        }
        catch (const std::exception& e)
        {
            printf("Call failed: %s", e.what());
        }
    });
```
