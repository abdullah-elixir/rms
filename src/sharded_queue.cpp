//
// Created by muhammad-abdullah on 6/4/25.
//
#include<concurrent/BackOffIdleStrategy.h>
#include "sharded_queue.h"
#include "utils/time_utils.h"
#include "logger.h"
#include "baseline/Order.h"

ShardedQueue::ShardedQueue() : _buffer(buffer, MAX_RING_BUFFER_SIZE + aeron::concurrent::ringbuffer::RingBufferDescriptor::TRAILER_LENGTH), _ring_buffer(_buffer) {}

ShardedQueue::~ShardedQueue() = default;

void ShardedQueue::enqueue(aeron::concurrent::AtomicBuffer buffer, int32_t offset, int32_t length) {
    aeron::concurrent::BackoffIdleStrategy idleStrategy(100,1000);
    bool isWritten = false;
    auto start = std::chrono::high_resolution_clock::now();
    //int8_t msgType = buffer.getUInt8(offset);
    while(!isWritten) {
        isWritten = _ring_buffer.write(1, buffer, offset, length);
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
    _ring_buffer.read([&](int8_t msgType, aeron::concurrent::AtomicBuffer& buffer, int32_t offset, int32_t length)
    {
        baseline::MessageHeader _message_header;
        baseline::Order _order_decoder;

        _message_header.wrap(reinterpret_cast<char*>(buffer.buffer()), offset, 0, buffer.capacity());
        offset += _message_header.encodedLength();  // usually 8 bytes
        _order_decoder.wrapForDecode(reinterpret_cast<char*>(buffer.buffer()), offset, _message_header.blockLength(), _message_header.version(), buffer.capacity());

        if (_message_header.templateId() == 1) {
            Order order;
            //getting fixed block from sbe
            order.order_id = _order_decoder.order_id();
            order.account_id = _order_decoder.account_id();
            order.instrument_id = _order_decoder.instrument_id();
            order.quantity = _order_decoder.quantity();
            order.price = _order_decoder.price();
            //getting variable length from sbe
            order.symbol = _order_decoder.getSymbolAsString();
            order.side = _order_decoder.getSideAsString();

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

