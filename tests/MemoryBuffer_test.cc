#include <gtest/gtest.h>

#include <cstring>
#include <string>

#include "Utility/MemoryBuffer.h"

using wcbot::MemoryBuffer;

TEST(MemoryBufferCtor, StartsWithInitialCapacityAndZeroLength) {
  MemoryBuffer Buf;
  EXPECT_EQ(Buf.GetLength(), 0u);
  // Initial capacity is an implementation detail (currently 8 KiB); we only
  // assert it is positive — the contract that matters for callers is that
  // the buffer is usable straight away.
  EXPECT_GT(Buf.GetCapacity(), 0u);
  EXPECT_NE(Buf.GetBase(), nullptr);
}

TEST(MemoryBufferAppend, GrowsAndPreservesData) {
  MemoryBuffer Buf;
  Buf.Append("hello, ", 7);
  Buf.Append("world", 5);
  ASSERT_EQ(Buf.GetLength(), 12u);
  EXPECT_EQ(std::string(Buf.GetBase(), Buf.GetLength()), "hello, world");
}

TEST(MemoryBufferAppend, GrowsBeyondInitialCapacity) {
  MemoryBuffer Buf;
  // Pump enough bytes to force at least one realloc/double.
  std::string Big(64 * 1024, 'x');
  Buf.Append(Big.data(), Big.size());
  EXPECT_EQ(Buf.GetLength(), Big.size());
  EXPECT_GE(Buf.GetCapacity(), Big.size());
  EXPECT_EQ(std::memcmp(Buf.GetBase(), Big.data(), Big.size()), 0);
}

TEST(MemoryBufferAllocate, KeepsCapacityInSync) {
  // Regression for the bug where Allocate() did not update Capacity, which
  // silently broke MaxRecvBuffLength enforcement.
  MemoryBuffer Buf;
  const size_t StartCap = Buf.GetCapacity();
  // Request enough headroom to definitely outgrow the initial buffer.
  Buf.Allocate(StartCap * 4);
  EXPECT_GE(Buf.GetCapacity(), StartCap * 4);
}

TEST(MemoryBufferIncreaseLength, MovesCursorWithoutCopy) {
  MemoryBuffer Buf;
  // Place data manually then commit via IncreaseLength (mirrors libuv's
  // alloc/read pattern).
  std::strcpy(Buf.GetBase(), "abcd");
  Buf.IncreaseLength(4);
  EXPECT_EQ(Buf.GetLength(), 4u);
  EXPECT_EQ(std::string(Buf.GetBase(), Buf.GetLength()), "abcd");
}

TEST(MemoryBufferSwap, SwapsContent) {
  MemoryBuffer A, B;
  A.Append("AAAA", 4);
  B.Append("BBBBBB", 6);
  A.SwapMemory(B);
  EXPECT_EQ(std::string(A.GetBase(), A.GetLength()), "BBBBBB");
  EXPECT_EQ(std::string(B.GetBase(), B.GetLength()), "AAAA");
}

TEST(MemoryBufferNullTerminate, AppendsNulWithoutGrowingLength) {
  MemoryBuffer Buf;
  Buf.Append("foo", 3);
  Buf.SetNullTerminated();
  // Length should stay 3; the trailing '\0' is at GetBase()[3].
  EXPECT_EQ(Buf.GetLength(), 3u);
  EXPECT_EQ(Buf.GetBase()[3], '\0');
}
