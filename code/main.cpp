#include <unistd.h>

#include "coroutine/iomanager.h"
#include "http_server.h"

void run() {
    Address::ptr addr = IPv4Address::Create("0.0.0.0", 6688);

    std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>(true);
    while(!server->bind(addr)) ;
    server->start();
}

int main() {
    IOManager manager(4, true);
    manager.schedule(run);
} 
