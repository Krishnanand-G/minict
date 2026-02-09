#include "minitest.hpp"

namespace minitest {

std::vector<Test>& registry() {
    static std::vector<Test> r;
    return r;
}

int& failures() {
    static int n = 0;
    return n;
}

Register::Register(const char* n, void (*f)()) {
    Test t;
    t.name = n;
    t.fn = f;
    registry().push_back(t);
}

void fail(const char* f, int l, const std::string& m, bool) {
    ++failures();
    std::cerr << f << ":" << l << ": " << m << "\n";
}

int RUN_ALL_TESTS() {
    std::cout << "[==========] Running " << registry().size() << " tests\n";
    for (size_t i = 0; i < registry().size(); ++i) {
        int before = failures();
        std::cout << "[ RUN      ] " << registry()[i].name << "\n";
        registry()[i].fn();
        std::cout << (before == failures() ? "[       OK ] " : "[  FAILED  ] ")
                  << registry()[i].name << "\n";
    }
    std::cout << "[==========] " << registry().size() << " tests, "
              << failures() << " failures\n";
    return failures() ? 1 : 0;
}

}
