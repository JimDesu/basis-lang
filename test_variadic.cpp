#include <memory>
#include <vector>
#include <iostream>

using SPFN = std::shared_ptr<int>;

template<typename... Args>
SPFN test_func(Args&&... args) {
    return std::make_shared<int>(sizeof...(args));
}

int main() {
    auto a = std::make_shared<int>(1);
    auto b = std::make_shared<int>(2);
    auto c = std::make_shared<int>(3);
    
    auto result = test_func(a, b, c);
    std::cout << "Result: " << *result << std::endl;
    return 0;
}

