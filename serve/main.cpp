#include "Serve/serve.h"
#include <iostream>

int main() {
    Serve server;
    std::cout << "启动服务器..." << std::endl;
    server.start();
    return 0;
}
