#include <gtest/gtest.h>
#include "utils.hpp"

TEST(UtilsTest, IpToUintAndBack) {
    EXPECT_EQ(Utils::uintToIp(Utils::ipToUint("192.168.1.1")), "192.168.1.1");
    EXPECT_EQ(Utils::uintToIp(Utils::ipToUint("10.0.0.1")), "10.0.0.1");
    EXPECT_EQ(Utils::uintToIp(Utils::ipToUint("255.255.255.255")), "255.255.255.255");
    EXPECT_EQ(Utils::uintToIp(Utils::ipToUint("0.0.0.0")), "0.0.0.0");
}

TEST(UtilsTest, IpToUintValues) {
    EXPECT_EQ(Utils::ipToUint("192.168.1.1"), 0xC0A80101U);
    EXPECT_EQ(Utils::ipToUint("10.0.0.1"), 0x0A000001U);
    EXPECT_EQ(Utils::ipToUint("0.0.0.0"), 0U);
    EXPECT_EQ(Utils::ipToUint("255.255.255.255"), 0xFFFFFFFFU);
}

TEST(UtilsTest, UintToIp) {
    EXPECT_EQ(Utils::uintToIp(0xC0A80101U), "192.168.1.1");
    EXPECT_EQ(Utils::uintToIp(0x0A000001U), "10.0.0.1");
    EXPECT_EQ(Utils::uintToIp(0U), "0.0.0.0");
}

TEST(UtilsTest, ParseCIDR_24) {
    auto [start, end] = Utils::parseCIDR("192.168.1.0/24");
    EXPECT_EQ(Utils::uintToIp(start), "192.168.1.0");
    EXPECT_EQ(Utils::uintToIp(end), "192.168.1.255");
    EXPECT_EQ(end - start + 1, 256U);
}

TEST(UtilsTest, ParseCIDR_16) {
    auto [start, end] = Utils::parseCIDR("10.0.0.0/16");
    EXPECT_EQ(Utils::uintToIp(start), "10.0.0.0");
    EXPECT_EQ(Utils::uintToIp(end), "10.0.255.255");
    EXPECT_EQ(end - start + 1, 65536U);
}

TEST(UtilsTest, ParseCIDR_32) {
    auto [start, end] = Utils::parseCIDR("192.168.1.100/32");
    EXPECT_EQ(Utils::uintToIp(start), "192.168.1.100");
    EXPECT_EQ(Utils::uintToIp(end), "192.168.1.100");
    EXPECT_EQ(end - start + 1, 1U);
}

TEST(UtilsTest, ParseCIDR_8) {
    auto [start, end] = Utils::parseCIDR("10.0.0.0/8");
    EXPECT_EQ(Utils::uintToIp(start), "10.0.0.0");
    EXPECT_EQ(Utils::uintToIp(end), "10.255.255.255");
}

TEST(UtilsTest, ParseCIDR_22) {
    auto [start, end] = Utils::parseCIDR("172.16.4.0/22");
    EXPECT_EQ(Utils::uintToIp(start), "172.16.4.0");
    EXPECT_EQ(Utils::uintToIp(end), "172.16.7.255");
    EXPECT_EQ(end - start + 1, 1024U);
}

TEST(UtilsTest, IsValidIpv4) {
    EXPECT_TRUE(Utils::isValidIpv4("192.168.1.1"));
    EXPECT_TRUE(Utils::isValidIpv4("10.0.0.1"));
    EXPECT_TRUE(Utils::isValidIpv4("0.0.0.0"));
    EXPECT_TRUE(Utils::isValidIpv4("255.255.255.255"));

    EXPECT_FALSE(Utils::isValidIpv4(""));
    EXPECT_FALSE(Utils::isValidIpv4("not-an-ip"));
    EXPECT_FALSE(Utils::isValidIpv4("256.1.1.1"));
    EXPECT_FALSE(Utils::isValidIpv4("192.168.1"));
    EXPECT_FALSE(Utils::isValidIpv4("1.2.3.4.5"));
    EXPECT_FALSE(Utils::isValidIpv4("; rm -rf /"));
}
