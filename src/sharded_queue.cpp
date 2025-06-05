//
// Created by muhammad-abdullah on 6/4/25.
//
#include "sharded_queue.h"

#include "utils/logger.h"
#include "utils/time_utils.h"

ShardedQueue::ShardedQueue() : _buffer(buffer, MAX_RING_BUFFER_SIZE + aeron::concurrent::ringbuffer::RingBufferDescriptor::TRAILER_LENGTH), _ring_buffer(_buffer) {}

ShardedQueue::~ShardedQueue() = default;

void ShardedQueue::enqueue(aeron::concurrent::AtomicBuffer buffer, int32_t offset, int32_t length) {
    rms::utils::logInfo("inside enqueue");
    int8_t msgType = buffer.getUInt8(offset);
    std::cout<<"got msgType : "<<msgType<<std::endl;
    int isSuc = _ring_buffer.write(msgType, buffer, offset, length);
    if (!isSuc) {
        rms::utils::logError("failed to enqueue msgType");
    }
}
std::optional<std::variant<Order, TradeExecution>> ShardedQueue::dequeue() {
    std::optional<std::variant<Order, TradeExecution>> result;
    _ring_buffer.read([&](int8_t msgType, aeron::concurrent::AtomicBuffer& buffer, int32_t offset, int32_t length) {
        if (msgType == '1') {
            Order order;
            memcpy(&order, buffer.buffer() + offset + 1, length);
            rms::utils::logInfo("got order msg type, " + fmt::to_string(order.account_id));
            result.emplace(order);
        }
        else if (msgType == '2') {
            TradeExecution trade;
            memcpy(&trade, buffer.buffer() + offset + 1, length);
            result.emplace(trade);
        }
        else {
            rms::utils::logError("Unexpected msgType");
        }
    });
    return result;
}
int ShardedQueue::size(){
    return _ring_buffer.size();
}

