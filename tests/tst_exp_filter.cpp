#include <cxxkit/numerics/exp_filter.hpp>

#include <gtest/gtest.h>

namespace
{

TEST(ExpFilter, FirstApplyPassthrough)
{
    cxxkit::ExpFilter filter(0.9f);
    EXPECT_FLOAT_EQ(10.0f, filter.apply(1.0f, 10.0f));
}

TEST(ExpFilter, ExpOneThirdConvergence)
{
    // y(k) = alpha^exp * y(k-1) + (1 - alpha^exp) * sample; converges to the sample.
    cxxkit::ExpFilter filter(0.9f);
    filter.apply(1.0f, 0.0f);
    float filtered = 0.0f;
    for (int i = 0; i < 400; ++i)
    {
        filtered = filter.apply(1.0f / 3.0f, 100.0f);
    }
    EXPECT_NEAR(100.0f, filtered, 1e-3f);
}

TEST(ExpFilter, AlphaOnePassthrough)
{
    // alpha=1 means the new sample has zero weight: output sticks at the initial value.
    cxxkit::ExpFilter filter(1.0f);
    filter.apply(1.0f, 10.0f);
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_FLOAT_EQ(10.0f, filter.apply(1.0f, 999.0f));
    }
}

TEST(ExpFilter, MaxCapClamping)
{
    cxxkit::ExpFilter filter(0.9f, 50.0f);
    filter.apply(1.0f, 100.0f);
    EXPECT_FLOAT_EQ(50.0f, filter.filtered());
    EXPECT_FLOAT_EQ(50.0f, filter.apply(1.0f, 200.0f));
    EXPECT_FLOAT_EQ(50.0f, filter.filtered());
}

TEST(ExpFilter, ResetClearsState)
{
    cxxkit::ExpFilter filter(0.9f, 50.0f);
    filter.apply(1.0f, 100.0f);
    EXPECT_FLOAT_EQ(50.0f, filter.filtered());
    filter.reset(0.5f);
    EXPECT_EQ(cxxkit::ExpFilter::kValueUndefined, filter.filtered());
    filter.apply(1.0f, 7.0f);
    EXPECT_FLOAT_EQ(7.0f, filter.filtered());
    filter.apply(1.0f, 9.0f);
    EXPECT_FLOAT_EQ(8.0f, filter.filtered());
}

TEST(ExpFilter, UpdateBaseChangesAlpha)
{
    cxxkit::ExpFilter filter(1.0f);
    filter.apply(1.0f, 10.0f);
    filter.update_base(0.5f);
    EXPECT_FLOAT_EQ(15.0f, filter.apply(1.0f, 20.0f));
}

} // namespace
