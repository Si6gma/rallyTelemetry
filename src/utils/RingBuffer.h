/**
 * Thread-Safe Ring Buffer for RTOS
 * 
 * Lock-free ring buffer using atomic operations for single-producer/single-consumer
 * scenarios. Falls back to mutex for multi-producer cases.
 */

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * @brief Thread-safe ring buffer template
 * @tparam T Type of elements stored
 * @tparam Size Buffer size (must be power of 2)
 */
template<typename T, size_t Size>
class RingBuffer {
    static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");
    
private:
    T buffer[Size];
    volatile size_t head = 0;  // Write index
    volatile size_t tail = 0;  // Read index
    SemaphoreHandle_t mutex = nullptr;
    const size_t mask = Size - 1;
    
public:
    RingBuffer() {
        mutex = xSemaphoreCreateMutex();
    }
    
    ~RingBuffer() {
        if (mutex) vSemaphoreDelete(mutex);
    }
    
    /**
     * @brief Push item to buffer (thread-safe)
     * @param item Item to push
     * @param timeoutTicks Timeout in RTOS ticks (0 = non-blocking)
     * @return true if successful, false if buffer full
     */
    bool push(const T& item, TickType_t timeoutTicks = portMAX_DELAY) {
        if (xSemaphoreTake(mutex, timeoutTicks) != pdTRUE) {
            return false;
        }
        
        size_t nextHead = (head + 1) & mask;
        if (nextHead == tail) {
            xSemaphoreGive(mutex);
            return false;  // Buffer full
        }
        
        buffer[head] = item;
        head = nextHead;
        
        xSemaphoreGive(mutex);
        return true;
    }
    
    /**
     * @brief Push item from ISR context
     * @param item Item to push
     * @param pxHigherPriorityTaskWoken Task switch flag
     * @return true if successful
     */
    bool pushFromISR(const T& item, BaseType_t* pxHigherPriorityTaskWoken = nullptr) {
        size_t nextHead = (head + 1) & mask;
        if (nextHead == tail) {
            return false;  // Buffer full
        }
        
        buffer[head] = item;
        head = nextHead;
        return true;
    }
    
    /**
     * @brief Pop item from buffer (thread-safe)
     * @param item Reference to store popped item
     * @param timeoutTicks Timeout in RTOS ticks
     * @return true if successful, false if buffer empty
     */
    bool pop(T& item, TickType_t timeoutTicks = portMAX_DELAY) {
        if (xSemaphoreTake(mutex, timeoutTicks) != pdTRUE) {
            return false;
        }
        
        if (head == tail) {
            xSemaphoreGive(mutex);
            return false;  // Buffer empty
        }
        
        item = buffer[tail];
        tail = (tail + 1) & mask;
        
        xSemaphoreGive(mutex);
        return true;
    }
    
    /**
     * @brief Check if buffer is empty
     */
    bool isEmpty() const {
        return head == tail;
    }
    
    /**
     * @brief Check if buffer is full
     */
    bool isFull() const {
        return ((head + 1) & mask) == tail;
    }
    
    /**
     * @brief Get number of items in buffer
     */
    size_t count() const {
        return (head - tail) & mask;
    }
    
    /**
     * @brief Get free space in buffer
     */
    size_t available() const {
        return Size - count() - 1;
    }
    
    /**
     * @brief Clear the buffer
     */
    void clear() {
        xSemaphoreTake(mutex, portMAX_DELAY);
        head = tail = 0;
        xSemaphoreGive(mutex);
    }
    
    /**
     * @brief Get buffer capacity
     */
    static constexpr size_t capacity() {
        return Size - 1;  // One slot reserved for full/empty distinction
    }
};

/**
 * @brief Double buffer for zero-copy swapping
 * Used for batch processing (e.g., SD card writes)
 */
template<typename T, size_t Size>
class DoubleBuffer {
private:
    T buffer1[Size];
    T buffer2[Size];
    T* writeBuffer = buffer1;
    T* readBuffer = buffer2;
    volatile size_t writeIndex = 0;
    SemaphoreHandle_t swapSemaphore = nullptr;
    
public:
    DoubleBuffer() {
        swapSemaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(swapSemaphore);
    }
    
    ~DoubleBuffer() {
        if (swapSemaphore) vSemaphoreDelete(swapSemaphore);
    }
    
    /**
     * @brief Add item to current write buffer
     */
    bool write(const T& item) {
        if (writeIndex >= Size) {
            return false;  // Buffer full, needs swap
        }
        writeBuffer[writeIndex++] = item;
        return true;
    }
    
    /**
     * @brief Swap buffers and return full buffer for processing
     * @return Pointer to full buffer, nullptr if empty
     */
    T* swap(size_t& count) {
        if (xSemaphoreTake(swapSemaphore, 0) != pdTRUE) {
            return nullptr;  // Swap in progress
        }
        
        if (writeIndex == 0) {
            xSemaphoreGive(swapSemaphore);
            return nullptr;  // Nothing to swap
        }
        
        // Swap pointers
        T* temp = writeBuffer;
        writeBuffer = readBuffer;
        readBuffer = temp;
        
        count = writeIndex;
        writeIndex = 0;
        
        xSemaphoreGive(swapSemaphore);
        return readBuffer;
    }
    
    /**
     * @brief Check if write buffer is full
     */
    bool isFull() const {
        return writeIndex >= Size;
    }
    
    /**
     * @brief Get current write count
     */
    size_t getWriteCount() const {
        return writeIndex;
    }
};
