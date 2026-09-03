#include <cxxkit/numerics/percentile_filter.hpp>

#include <gtest/gtest.h>

#include <cstdint>

TEST(PercentileFilterTest, SingleElementAllPercentiles)
{
    cxxkit::PercentileFilter<int> filterMin(0.0f);
    cxxkit::PercentileFilter<int> filterMid(0.5f);
    cxxkit::PercentileFilter<int> filterMax(1.0f);
    filterMin.insert(3);
    filterMid.insert(3);
    filterMax.insert(3);
    EXPECT_EQ(3, filterMin.get_percentile_value());
    EXPECT_EQ(3, filterMid.get_percentile_value());
    EXPECT_EQ(3, filterMax.get_percentile_value());
}

TEST(PercentileFilterTest, MedianOfIncreasingSequence)
{
    cxxkit::PercentileFilter<int> filter(0.5f);
    for (int i = 1; i <= 100; ++i)
    {
        filter.insert(i);
    }
    EXPECT_EQ(50, filter.get_percentile_value());
}

TEST(PercentileFilterTest, EraseExistingReturnsTrue)
{
    cxxkit::PercentileFilter<int> filter(0.5f);
    filter.insert(1);
    filter.insert(2);
    filter.insert(3);
    EXPECT_TRUE(filter.erase(2));
    EXPECT_EQ(
        1,
        filter.get_percentile_value()); // median of {1, 2, 3} was 2; after erasing 2: {1, 3}, index trunc(0.5*1)=0 -> 1
}

TEST(PercentileFilterTest, EraseMissingReturnsFalse)
{
    cxxkit::PercentileFilter<int> filter(0.5f);
    filter.insert(1);
    filter.insert(2);
    EXPECT_FALSE(filter.erase(42));
    EXPECT_FALSE(filter.erase(42)); // still missing

    cxxkit::PercentileFilter<int> emptyFilter(0.5f);
    EXPECT_FALSE(emptyFilter.erase(1));
}

TEST(PercentileFilterTest, MinAndMaxBoundaries)
{
    cxxkit::PercentileFilter<int> filterMin(0.0f);
    cxxkit::PercentileFilter<int> filterMax(1.0f);
    for (int i = 1; i <= 100; ++i)
    {
        filterMin.insert(i);
        filterMax.insert(i);
    }
    EXPECT_EQ(1, filterMin.get_percentile_value());
    EXPECT_EQ(100, filterMax.get_percentile_value());
}

TEST(PercentileFilterTest, ResetSemantics)
{
    cxxkit::PercentileFilter<int> filter(0.5f);
    filter.insert(10);
    filter.insert(20);
    filter.insert(30);
    ASSERT_EQ(20, filter.get_percentile_value());

    filter.reset();
    // Empty set: follows the source behavior (returns T()).
    EXPECT_EQ(0, filter.get_percentile_value());

    // Usable again after reset.
    filter.insert(7);
    EXPECT_EQ(7, filter.get_percentile_value());
    filter.insert(9);
    filter.insert(11);
    EXPECT_EQ(9, filter.get_percentile_value());
}
