
#include "pyyjson.h"
#include "test.h"
#include "test_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#if defined(_MSC_VER)
#    include <intrin.h>
#    define cpuid_count(info, x) __cpuidex(info, x, 0)
#    define cpuid(info, x) __cpuid(info, x)
#else
#    include <cpuid.h>

force_inline void cpuid_count(int *info, int x) {
    __cpuid_count(x, 0, info[0], info[1], info[2], info[3]);
}

force_inline void cpuid(int *info, int x) {
    __cpuid(x, info[0], info[1], info[2], info[3]);
}
#endif

bool _SupportAVX512 = false;
bool _SupportAVX2 = false;

void check_avx512(void) {
    int info[4];
    cpuid_count(info, 7);
    int ebx = info[1];
    _SupportAVX512 = (ebx & (1 << 16)) && (ebx & (1 << 30));
}

void check_avx2(void) {
    int info[4];
    cpuid_count(info, 7);
    int ebx = info[1];
    _SupportAVX2 = ebx & (1 << 5);
}

#define _RED "\033[31m"
#define _GREEN "\033[32m"
#define _YELLOW "\033[33m"
#define _CLEAR "\033[0m"

typedef struct TestCounter {
    int total_count;
    int skipped_count;
    int passed_count;
} TestCounter;

bool wrap_run_test(bool (*func)(void), const char *name, TestCounter *counter) {
    printf("RUNNING TEST: %s", name);
    bool ret = func();
    counter->total_count++;
    if (ret) {
        printf("\t\t\t" _GREEN "PASSED" _CLEAR "\n");
        counter->passed_count++;
    } else {
        printf("\t\t\t" _RED "!!!!!FAILED" _CLEAR "\n");
    }
    return ret;
}

#define RUN_ONE_TEST(_name) wrap_run_test(_name, #_name, &counter)
#define SKIP_ONE_TEST(_name) \
    counter.total_count++;   \
    counter.skipped_count++; \
    printf("RUNNING TEST: %s\t\t\t" _YELLOW "SKIPPED" _CLEAR "\n", #_name);
#if BUILD_MULTI_LIB
#    define RUN_TESTS(_name)                                \
        do {                                                \
            if (support_avx512) {                           \
                check_pass &= RUN_ONE_TEST(_name##_avx512); \
            } else {                                        \
                SKIP_ONE_TEST(_name##_avx512)               \
            }                                               \
            if (support_avx2) {                             \
                check_pass &= RUN_ONE_TEST(_name##_avx2);   \
            } else {                                        \
                SKIP_ONE_TEST(_name##_avx2)                 \
            }                                               \
            check_pass &= RUN_ONE_TEST(_name##_sse2);       \
        } while (0)
#else
#    define RUN_TESTS(_name) check_pass &= RUN_ONE_TEST(_name);
#endif

void show_test_counter(TestCounter *counter) {
    int failed = counter->total_count - (counter->passed_count + counter->skipped_count);
    printf("==================================================================================\n");
    if (!failed) {
        printf(_GREEN "Summary: ALL PASSED, %d tests in total, %d passed, %d skipped." _CLEAR "\n", counter->total_count, counter->passed_count, counter->skipped_count);
    } else {
        printf(_RED "Summary: %d tests in total, %d passed, %d skipped, %d failed." _CLEAR "\n", counter->total_count, counter->passed_count, counter->skipped_count, failed);
    }
}

bool run_c_tests(void) {
    bool support_avx512 = _SupportAVX512;
    bool support_avx2 = _SupportAVX2;

    bool check_pass = true;
    TestCounter counter;
    ZERO_FILL(counter);

    RUN_TESTS(test_elevate_1_2_to_128);
    RUN_TESTS(test_elevate_1_4_to_128);
    RUN_TESTS(test_elevate_2_4_to_128);

    show_test_counter(&counter);
    return check_pass;
}

int main(int argc, char **argv) {
    if (!initialize_cpython()) {
        fprintf(stderr, "Fail to initialize");
        return 1;
    }
    srand((u32)time(NULL));
    check_avx2();
    check_avx512();
    //
    int ret = 0;
    //
    bool run_test_result = run_c_tests();
    if (!run_test_result) ret = 1;

done:
    Py_Finalize();
    return ret;
}
