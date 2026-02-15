#include "greeting.h"
#include <gtest/gtest.h>

class GreeetingTest : public testing::Test {
protected:
  void SetUp() override {}
  void TearDown() override {}
};

TEST_F(GreeetingTest, greetingTest) { EXPECT_EQ(greeting(), "Hello World!"); }
