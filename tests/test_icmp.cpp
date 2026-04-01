#include <gtest/gtest.h>
#include "icmp.hpp"
#include <cstring>

TEST(IcmpTest, ChecksumZeroBuffer) {
    // A zero-filled buffer should produce a checksum of 0xFFFF
    char buf[64]{};
    uint16_t cs = Icmp::checksum(buf, sizeof(buf));
    EXPECT_EQ(cs, 0xFFFF);
}

TEST(IcmpTest, ChecksumKnownValue) {
    // Simple test: checksum of a buffer with known content
    uint16_t data[] = {0x0001, 0x0002, 0x0003, 0x0004};
    uint16_t cs = Icmp::checksum(data, sizeof(data));
    // Sum = 0x000A, complement = 0xFFF5
    EXPECT_EQ(cs, 0xFFF5);
}

TEST(IcmpTest, ChecksumOddLength) {
    // Odd-length buffer: 3 bytes
    uint8_t data[] = {0x01, 0x02, 0x03};
    uint16_t cs = Icmp::checksum(data, 3);
    // Should handle the trailing byte correctly
    EXPECT_NE(cs, 0); // Just verify it doesn't crash and produces non-zero
}

TEST(IcmpTest, ChecksumSymmetry) {
    // Checksum of data + its checksum appended should be 0 (or 0xFFFF depending on implementation)
    char buf[8]{};
    buf[0] = 0x12;
    buf[1] = 0x34;
    buf[2] = 0x56;
    buf[3] = 0x78;
    uint16_t cs = Icmp::checksum(buf, 4);

    // Place checksum in the buffer and verify
    buf[4] = cs & 0xFF;
    buf[5] = (cs >> 8) & 0xFF;
    uint16_t verify = Icmp::checksum(buf, 6);
    // With correct checksum implementation, this should validate
    // The result should be 0xFFFF or 0x0000
    EXPECT_TRUE(verify == 0xFFFF || verify == 0x0000);
}
