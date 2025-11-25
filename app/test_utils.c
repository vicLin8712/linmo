/* Utils test suit
 *
 * Current coverages:
 * - ctz:
 *   * Invalid input, v=0
 *   * Single-bit value input: 0b00...01, 0b00...10,...,0b10...00
 *   * Random value input: Lowest bit fix with upper bits random
 */

#include <linmo.h>

#define TEST_PASS 1
#define TEST_FAIL 0

/* Test result tracking */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test assertion macro */
#define ASSERT_TEST(condition, test_name)     \
    do {                                      \
        tests_run++;                          \
        if (condition) {                      \
            tests_passed++;                   \
            printf("[PASS] %s\n", test_name); \
        } else {                              \
            tests_failed++;                   \
            printf("[FAIL] %s\n", test_name); \
        }                                     \
    } while (0)

/* Test 1: De Bruijn-based ctz unit test */
void test_ctz(void)
{
    /* 1. v = 0, invalid input case */
    ASSERT_TEST(ctz(0) == -1, "invalid ctz input");

    bool single_bit_test = true;
    /* 2. Single-bit check: exact correctness */
    for (int bit = 0; bit < 32; bit++) {
        uint32_t v = (1u << bit);
        if (ctz(v) != bit) {
            single_bit_test = false;
            break;
        }
    }
    ASSERT_TEST(single_bit_test == true, "Single-bit ctz test");

    bool random_test = true;
    /* 3. Random noise tests for lowest bit; the lowest = 31 case has been
     * verified in test 2, single-bit check. */
    for (int lowest = 0; lowest < 31 && random_test; lowest++) {
        /* run multiple random tests */
        for (int i = 0; i < 200; i++) {
            uint32_t base = (1u << lowest);
            uint32_t v;

            uint32_t noise = random();

            /* Clear bits below 'lowest'; keep bits from 'lowest' and above */
            uint32_t mask = ~((1u << lowest) - 1u);
            uint32_t high = noise & mask;

            v = base | high;

            if (ctz(v) != lowest) {
                random_test = false;
                break;
            }
        }
    }
    ASSERT_TEST(random_test == true, "Random value ctz test");
}

void test_runner(void)
{
    printf("\n=== Utils Test Suite ===\n");
    printf("Testing: ctz\n\n");

    test_ctz();

    printf("\n=== Test Summary ===\n");
    printf("Tests run:    %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n[SUCCESS] All tests passed!\n");
    } else {
        printf("\n[FAILURE] %d test(s) failed!\n", tests_failed);
    }
}

int32_t app_main(void)
{
    /* Flush all messages in logger */
    mo_logger_flush();
    test_runner();

    /* Shutdown QEMU cleanly after tests complete */
    *(volatile uint32_t *) 0x100000U = 0x5555U;

    /* Fallback: keep task alive if shutdown fails (non-QEMU environments) */
    while (1)
        ; /* Idle loop */

    return 0;
}
