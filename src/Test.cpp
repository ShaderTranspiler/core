#include "Test.h"

#include <iostream>

namespace stc {

void Test::sayHi() {
    std::cout << "Hi!" << std::endl;
}

int Test::retOne() {
    return 1;
}

} // namespace stc