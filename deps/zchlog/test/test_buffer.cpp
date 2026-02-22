#include <gtest/gtest.h>
#include "Buffer.hpp"

TEST(BufferTest, InitialState) {
    zch::Buffer buffer;
    EXPECT_TRUE(buffer.Empty());
    EXPECT_EQ(buffer.ReadableSize(), 0);
    EXPECT_EQ(buffer.WriteableSize(), zch::defaultBufferSize);
}

TEST(BufferTest, PushAndRead) {
    zch::Buffer buffer;
    const char* data = "hello world";
    size_t len = strlen(data);
    
    buffer.Push(data, len);
    
    EXPECT_FALSE(buffer.Empty());
    EXPECT_EQ(buffer.ReadableSize(), len);
    EXPECT_EQ(strncmp(buffer.Start(), data, len), 0);
    
    buffer.MoveReadIdx(len);
    EXPECT_TRUE(buffer.Empty());
    EXPECT_EQ(buffer.ReadableSize(), 0);
}

TEST(BufferTest, Reset) {
    zch::Buffer buffer;
    buffer.Push("test", 4);
    buffer.reset();
    
    EXPECT_TRUE(buffer.Empty());
    EXPECT_EQ(buffer.ReadableSize(), 0);
    EXPECT_EQ(buffer.WriteableSize(), zch::defaultBufferSize);
}

TEST(BufferTest, Swap) {
    zch::Buffer b1;
    zch::Buffer b2;
    
    b1.Push("buffer1", 7);
    b2.Push("b2", 2);
    
    b1.swap(b2);
    
    EXPECT_EQ(b1.ReadableSize(), 2);
    EXPECT_EQ(b2.ReadableSize(), 7);
    EXPECT_EQ(strncmp(b1.Start(), "b2", 2), 0);
    EXPECT_EQ(strncmp(b2.Start(), "buffer1", 7), 0);
}
