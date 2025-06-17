//
// Created by muhammad-abdullah on 6/4/25.
//
#include<concurrent/BackOffIdleStrategy.h>
#include "sharded_queue.h"
#include "utils/time_utils.h"
#include "logger.h"

ShardedQueue::ShardedQueue() : _buffer(buffer, MAX_RING_BUFFER_SIZE + aeron::concurrent::ringbuffer::RingBufferDescriptor::TRAILER_LENGTH), _ring_buffer(_buffer) {}

ShardedQueue::~ShardedQueue() = default;

void ShardedQueue::enqueue(aeron::concurrent::AtomicBuffer buffer, int32_t offset, int32_t length) {
    aeron::concurrent::BackoffIdleStrategy idleStrategy(100,1000);
    bool isWritten = false;
    auto start = std::chrono::high_resolution_clock::now();
    int8_t msgType = buffer.getUInt8(offset);
    while(!isWritten) {
        isWritten = _ring_buffer.write(msgType, buffer, offset, length);
        if (isWritten) {
            return;
        }
        if (std::chrono::high_resolution_clock::now() - start >= std::chrono::microseconds(50)) {
            std::cerr << "retry timeout" << std::endl;
            return;
        }
        idleStrategy.idle();
    }
}
std::optional<std::variant<Order, TradeExecution>> ShardedQueue::dequeue() {
    std::optional<std::variant<Order, TradeExecution>> result;
    _ring_buffer.read([&](int8_t msgType, aeron::concurrent::AtomicBuffer& buffer, int32_t offset, int32_t length) {
        if (msgType == '1') {
            Order order;
            if (length >= sizeof(order)) {
                memcpy(&order, buffer.buffer() + offset + 1, sizeof(order));
            }
            result.emplace(order);
        }
        else if (msgType == '2') {
            TradeExecution trade;
            if (length >= sizeof(trade)) {
                memcpy(&trade, buffer.buffer() + offset + 1, sizeof(trade));
            }
            result.emplace(trade);
        }
        else {
            std::cerr << "Unexpected msgType" << std::endl;
        }
    });
    return result;
}
int ShardedQueue::size(){
    return _ring_buffer.size();
}

