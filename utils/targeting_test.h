#ifndef __TARGETING_TEST_H__
#define __TARGETING_TEST_H__
#include "gtest/gtest.h"

class TargetingTest : public ::testing::Test {

	public:
		TargetingTest()
		{
		}
		~TargetingTest()
		{
		}
};

TEST_F(TargetingTest, Initialization)
{
	ASSERT_EQ(1, 2);
}
#endif
