//
// Created by muhammad-abdullah on 6/4/25.
//

#ifndef SHARDED_QUEUE_H
#define SHARDED_QUEUE_H
#include <queue>
#include <concurrent/ringbuffer/OneToOneRingBuffer.h>
#include<variant>
#include<optional>

#include "utils/params.h"
#include "data_types.h"

#endif //SHARDED_QUEUE_H

class ShardedQueue {
    public:
    ShardedQueue();
    ~ShardedQueue();
    void enqueue(aeron::concurrent::AtomicBuffer, int32_t, int32_t);
    std::optional<std::variant<Order, TradeExecution>> dequeue();
    int size();
    private:
    std::array<uint8_t, MAX_RING_BUFFER_SIZE + aeron::concurrent::ringbuffer::RingBufferDescriptor::TRAILER_LENGTH> buffer;
    aeron::concurrent::AtomicBuffer _buffer;
    aeron::concurrent::ringbuffer::OneToOneRingBuffer _ring_buffer;
};