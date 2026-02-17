#include <Arduino.h>
#include <unity.h>
#include "../../src/utils/RingBuffer.h"

struct TestData {
    uint32_t timestamp;
    float value;
};

RingBuffer<TestData, 16> testBuffer;

void setUp(void) {
    testBuffer.clear();
}

void tearDown(void) {
    // Empty
}

void test_buffer_starts_empty(void) {
    TEST_ASSERT_TRUE(testBuffer.isEmpty());
    TEST_ASSERT_FALSE(testBuffer.isFull());
    TEST_ASSERT_EQUAL(0, testBuffer.count());
}

void test_push_pop_single(void) {
    TestData data = {12345, 3.14f};
    
    TEST_ASSERT_TRUE(testBuffer.push(data));
    TEST_ASSERT_EQUAL(1, testBuffer.count());
    TEST_ASSERT_FALSE(testBuffer.isEmpty());
    
    TestData result;
    TEST_ASSERT_TRUE(testBuffer.pop(result));
    TEST_ASSERT_EQUAL(12345, result.timestamp);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 3.14f, result.value);
    TEST_ASSERT_TRUE(testBuffer.isEmpty());
}

void test_buffer_fills_correctly(void) {
    // Fill buffer (capacity is 15, not 16, due to one slot reserved)
    for (int i = 0; i < 15; i++) {
        TestData data = {(uint32_t)i, (float)i * 1.5f};
        TEST_ASSERT_TRUE(testBuffer.push(data));
    }
    
    TEST_ASSERT_TRUE(testBuffer.isFull());
    TEST_ASSERT_EQUAL(15, testBuffer.count());
    
    // Next push should fail
    TestData overflow = {999, 999.0f};
    TEST_ASSERT_FALSE(testBuffer.push(overflow, 0)); // Non-blocking
}

void test_fifo_order(void) {
    // Push multiple items
    for (int i = 0; i < 5; i++) {
        TestData data = {(uint32_t)i, (float)i * 10.0f};
        testBuffer.push(data);
    }
    
    // Verify FIFO order
    for (int i = 0; i < 5; i++) {
        TestData result;
        TEST_ASSERT_TRUE(testBuffer.pop(result));
        TEST_ASSERT_EQUAL(i, result.timestamp);
        TEST_ASSERT_FLOAT_WITHIN(0.001, i * 10.0f, result.value);
    }
}

void test_clear_buffer(void) {
    // Add some data
    for (int i = 0; i < 5; i++) {
        TestData data = {(uint32_t)i, (float)i};
        testBuffer.push(data);
    }
    
    TEST_ASSERT_EQUAL(5, testBuffer.count());
    
    // Clear and verify
    testBuffer.clear();
    TEST_ASSERT_TRUE(testBuffer.isEmpty());
    TEST_ASSERT_EQUAL(0, testBuffer.count());
}

void test_available_space(void) {
    TEST_ASSERT_EQUAL(15, testBuffer.available());
    
    testBuffer.push({1, 1.0f});
    TEST_ASSERT_EQUAL(14, testBuffer.available());
    
    // Fill it up
    while (!testBuffer.isFull()) {
        testBuffer.push({0, 0.0f});
    }
    TEST_ASSERT_EQUAL(0, testBuffer.available());
}

void test_pop_empty_returns_false(void) {
    TestData result;
    TEST_ASSERT_FALSE(testBuffer.pop(result, 0)); // Non-blocking
}

void setup() {
    UNITY_BEGIN();
    
    RUN_TEST(test_buffer_starts_empty);
    RUN_TEST(test_push_pop_single);
    RUN_TEST(test_buffer_fills_correctly);
    RUN_TEST(test_fifo_order);
    RUN_TEST(test_clear_buffer);
    RUN_TEST(test_available_space);
    RUN_TEST(test_pop_empty_returns_false);
    
    UNITY_END();
}

void loop() {
    // Empty
}
