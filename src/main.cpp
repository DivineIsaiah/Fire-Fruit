#include <iostream>

#include "app/Application.h"

int main() {
    try {
        sde::Application app;
        app.run();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
}
