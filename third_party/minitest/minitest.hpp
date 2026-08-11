#pragma once
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace minitest {

struct Test {
    std::string name;
    std::function<void()> fn;
};

std::vector<Test>& registry();
int& failures();
void fail(const char* file, int line, const std::string& msg);

struct Register {
    Register(const char* n, void (*f)());
};

template <class A, class B>
void eq(const A& a, const B& b, const char* as, const char* bs, const char* f, int l) {
    if (!(a == b)) {
        std::ostringstream o;
        o << "expected " << as << " == " << bs << " (" << a << " vs " << b << ")";
        fail(f, l, o.str());
    }
}

int RUN_ALL_TESTS();

}

#define TEST(S, N) \
    void S##_##N(); \
    static minitest::Register reg_##S##_##N(#S "." #N, S##_##N); \
    void S##_##N()

#define EXPECT_TRUE(x) do { if (!(x)) minitest::fail(__FILE__, __LINE__, "expected true: " #x); } while (0)
#define EXPECT_FALSE(x) EXPECT_TRUE(!(x))
#define EXPECT_EQ(a, b) minitest::eq((a), (b), #a, #b, __FILE__, __LINE__)
