- [gRPC-Quick](#grpc-quick)
    - [Design Goals](#design-goals)
    - [Examples](#examples)
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

### Simple unary call

Here's an example shows how to build a server to handle unary calls, and build a client to perform unary calls.  
We define a proto service and related messages as follows:

```protobuf
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
function to the above server object.  
Note that:  
The RPC handler is registered with the generated function which in form of 'AsyncService::Request<RpcName>`;  
The handler function is called in thread other than your current thread, you should take care of the thread safety.

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
