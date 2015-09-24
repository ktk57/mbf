#include "gtest/gtest.h"
#include <cstring>
#include <cassert>
#include <algorithm>
#include <memory>
//#include "glog/logging.h"




uint32_t mfloor(uint32_t dividend, uint32_t divisor)
{
	assert(divisor != 0);
	if (dividend == 0) {
		return 0;
	}
	return dividend / divisor;
}

// returns ceil(dividend/divisor)
uint32_t mceil(uint32_t dividend, uint32_t divisor)
{
	assert(divisor != 0);
	if (dividend == 0) {
		return 0;
	}
	return (dividend - 1)/divisor + 1;
}

TEST(mfloorTest, nonZero)
{
	ASSERT_EQ(2, mfloor(8, 3));
	ASSERT_EQ(2, mfloor(6, 3));
	ASSERT_EQ(0, mfloor(1, 3));
}

TEST(mfloorTest, forZero)
{
	ASSERT_EQ(0, mfloor(1, 3));
	ASSERT_EQ(0, mfloor(2, 3));
	ASSERT_EQ(0, mfloor(3, 4));
}

TEST(mceilTest, forZero)
{
	ASSERT_EQ(0, mceil(0, 3));
	ASSERT_EQ(0, mceil(0, 3));
	ASSERT_EQ(0, mceil(0, 4));
}

TEST(mceilTest, nonZero)
{
	ASSERT_EQ(1, mceil(1, 3));
	ASSERT_EQ(1, mceil(2, 3));
	ASSERT_EQ(1, 2);
}


int main(int argc, char *argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
