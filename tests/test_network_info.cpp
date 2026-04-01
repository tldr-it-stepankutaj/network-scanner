#include <gtest/gtest.h>
#include "network_info.hpp"
#include "utils.hpp"

TEST(NetworkInfoTest, GetSubnet24) {
    EXPECT_EQ(getSubnet24("192.168.1.100"), "192.168.1.0/24");
    EXPECT_EQ(getSubnet24("10.0.0.1"), "10.0.0.0/24");
    EXPECT_EQ(getSubnet24("172.16.5.42"), "172.16.5.0/24");
}

TEST(NetworkInfoTest, IpToCIDR_24) {
    std::string cidr = ipToCIDR("192.168.1.100", "255.255.255.0");
    EXPECT_EQ(cidr, "192.168.1.0/24");
}

TEST(NetworkInfoTest, IpToCIDR_16) {
    std::string cidr = ipToCIDR("10.0.5.42", "255.255.0.0");
    EXPECT_EQ(cidr, "10.0.0.0/16");
}

TEST(NetworkInfoTest, IpToCIDR_22) {
    std::string cidr = ipToCIDR("172.16.5.42", "255.255.252.0");
    EXPECT_EQ(cidr, "172.16.4.0/22");
}

TEST(NetworkInfoTest, IpToCIDR_8) {
    std::string cidr = ipToCIDR("10.123.45.67", "255.0.0.0");
    EXPECT_EQ(cidr, "10.0.0.0/8");
}

TEST(NetworkInfoTest, IpToCIDR_32) {
    std::string cidr = ipToCIDR("192.168.1.1", "255.255.255.255");
    EXPECT_EQ(cidr, "192.168.1.1/32");
}
