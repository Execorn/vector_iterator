#include "../include/X17Vector.hpp"
#include "printf.hpp"

using namespace X17;

void testvector_nonbool() {
    vector<int> test_v(10, 5);
    for (size_t i = 0; i < 50; ++i) {
        test_v.push_back(i);
    }
    for (size_t idx = 0; idx < 50; ++idx) {
        std::cout << test_v[idx] << " ";
    }

    std::cout << std::endl;
}

int main() {
    X17::m_printf("test №%i vectors\n", 10);    

    testvector_nonbool();

    return 0;
}

 
// seminar:
// 1) add container pointer to iterator class (to access container from iterator if container is not given as an argument)
// 2) make iterator_trait for my class
// 3) make our class work with std::copy and range_based_for 
// 4) write cin and cout
// 5) write an iterator