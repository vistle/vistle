#include <vistle/util/enumarray.h>
#include <vistle/util/stopwatch.h>
#include <iostream>
enum TestEnum { T1, T2, LastDummy };

struct TestStruct {
    TestStruct() = default;
    TestStruct(int i): m_i(i) {}
    int m_i = 0;
};

int main()
{
    vistle::EnumArray<TestStruct, TestEnum> a{TestStruct{4}, TestStruct{5}};
    if (a[TestEnum::T1].m_i != 4 || a[TestEnum::T2].m_i != 5) {
        std::cerr << "test failed" << std::endl;
        abort();
    }


    a = vistle::EnumArray<TestStruct, TestEnum>();
    a[T1] = TestStruct{3};
    a[T2] = TestStruct{1};
    if (a[TestEnum::T1].m_i != 3 || a[TestEnum::T2].m_i != 1) {
        std::cerr << "test failed" << std::endl;
        abort();
    }


    std::cerr << "test succeeded" << std::endl;
    size_t numIter = 1000000;
    int tot = 0;
    {
        std::array<TestStruct, 2> sa{TestStruct{3}, TestStruct{1}};
        int sum = 0;
        vistle::StopWatch s{"std::array took"};
        for (size_t i = 0; i < numIter; i++) {
            sum += sa[static_cast<int>(T1)].m_i;
            sum += sa[static_cast<int>(T2)].m_i;
        }
        tot += sum;
    }
    {
        vistle::StopWatch s{"enumArray took"};
        int sum = 0;
        for (size_t i = 0; i < numIter; i++) {
            sum += a[T1].m_i;
            sum += a[T2].m_i;
        }
        tot += sum;
    }
    return tot == 0 ? 1 : 0;
}
