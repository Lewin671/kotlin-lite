#include <iostream>
#include "compiler.hpp"

int main(int, char**){
    std::cout << kotlin_lite::getGreeting() << std::endl;
    return 0;
}