#include <gtest/gtest.h>
#include "GammaCommon/TVector3.h"

using Gamma::TVector3;

TEST(GammaMath_Unit, DistSqr_3_4_5)
{
    TVector3<float> a(3.f, 4.f, 0.f);
    TVector3<float> origin(0.f, 0.f, 0.f);
    EXPECT_FLOAT_EQ(a.DistSqr(origin), 25.f);
}
