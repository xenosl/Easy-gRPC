- [gRPC-Quick](#grpc-quick)
    - [Design Goals](#design-goals)
    - [Examples](#examples)
        - [Simple unary call](#simple-uary-call)

# gRPC-Quick

## Design Goals

- **Automatic code generation**. gRPC uses protobuf with a custom plugin to generates codes for serialization and rpc methods. It is tedious and error-prone to
  do it manually, the project provides convenient CMake functions to achieve it automatically.
- **Simplify rpc call & handling**. Simplify the use of remote call/handling with arbitrary rpc types with all types of rpc methods, i.e. unary call,
  client/server streaming, bidirectional streaming.

## Examples

### Simple unary call

For example, there is a proto file defines a service:

```protobuf
syntax = "proto3";

package ShuHai.gRPC.Examples.Proto.Task;

service Rpc
{
    rpc StartTask(StartTaskRequest) returns(StartTaskReply) {}
}

message StartTaskRequest
{
    string codename = 1;
}

message StartTaskReply
{
    bool started = 1;
}
```

A custom server class corresponding to the above server:

```c++
using namespace ShuHai::gRPC;
using RpcServer = Server::AsyncServer<Proto::Task::Rpc::AsyncService>;
```

The implementation of actual business logic can be registered with the generated function ``AsyncService::Request<RpcName>``:

```c++
static void StartTask(const Proto::Task::StartTaskRequest& request, Proto::Task::StartTaskReply& reply)
{
    // Implement business logic here.
}

RpcServer server(12345);
server.registerCallHandler(&Service::RequestStartTask, &StartTask);
```

Finally, start the server:

```c++
server.start();
```
