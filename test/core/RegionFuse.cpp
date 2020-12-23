//
//  RegionFuse.cpp
//  MNNTests
//
//  Created by wangzhaode on 2020/9/21.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "MNNTestSuite.h"
#include "MNN_generated.h"
#include <MNN/Tensor.hpp>
#include "core/TensorUtils.hpp"

using namespace MNN;
class RegionFuseTest : public MNNTestCase {
public:
    using Region = Tensor::InsideDescribe::Region;
    virtual ~RegionFuseTest() = default;
    virtual bool run() {
        constexpr int N = 9;
        // [src_offset, src_stride_0_1_2, dst_offset, dst_stride_0_1_2, size_0_1_2]
        int data[N*3][11] = {
            // 2D-transpose + 2D-transpose = memcpy: [1, 4, 16] => [1, 16, 4] => [1, 4, 16]
            {0, 1, 1, 16, 0, 1, 4, 1, 1, 16, 4},
            {0, 1, 1, 4, 0, 1, 16, 1, 1, 4, 16},
            {0, 1, 16, 1, 0, 1, 16, 1, 1, 4, 16},
            // transpose + memcpy = transpose: [1, 4, 16] => [1, 16, 4] => [16, 1, 4]
            {0, 1, 1, 16, 0, 1, 4, 1, 1, 16, 4},
            {0, 1, 1, 1, 0, 1, 1, 1, 16, 1, 4},
            {0, 1, 1, 16, 0, 1, 4, 1, 1, 16, 4},
            // transpose + transpose' = transpose'': [3, 4, 5] => [5, 3, 4] => [4, 5, 3]
            {0, 1, 1, 5, 0, 1, 12, 1, 1, 5, 12},
            {0, 1, 1, 4, 0, 1, 15, 1, 1, 4, 15},
            {0, 5, 1, 20, 0, 15, 3, 1, 4, 5, 3},
            // memcpy + memcpy' = memcpy'': offset:2 => offset:3 => offser:6+2-3=5
            {2, 1, 1, 1, 3, 1, 1, 1, 1, 1, 16},
            {6, 1, 1, 1, 0, 1, 1, 1, 1, 1, 16},
            {5, 1, 1, 1, 0, 1, 1, 1, 1, 1, 16},
            // transpose + slice (offset align) => [3, 3, 4] => [3, 4, 3] => [2, 4, 3]
            {0, 12, 1, 4, 0, 12, 3, 1, 3, 4, 3},
            {12, 36, 3, 1, 0, 24, 3, 1, 1, 8, 3},
            {12, 12, 1, 4, 0, 12, 3, 1, 1, 8, 3},
            // transpose + slice (offset dont align) => [3, 3, 4] => [3, 4, 3] => [1, 6, 3] <can't fuse!>
            {0, 12, 1, 4, 0, 12, 3, 1, 3, 4, 3},
            {18, 36, 3, 1, 0, 18, 3, 1, 1, 6, 3},
            {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
            // copy + expand (src < dst) => [34491] => [34645] => [34645, 2] <can't fuse!>
            {0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 34491},
            {0, 1, 1, 1, 0, 2, 1, 1, 34645, 1, 1},
            {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
            // transpose + slice: [3, 256, 940] => [3, 940, 256] => [1, 256, 940] (expand_val = 1)
            {0, 240640, 1, 940, 0, 240640, 256, 1, 3, 940, 256},
            {0, 1, 256, 1, 0, 1, 768, 1, 1, 940, 256},
            {0, 240640, 1, 940, 0, 721920, 768, 1, 1, 940, 256},
            // transpose + slice (stride = 0) <can't fuse>
            {0, 4608, 1, 36, 0, 4608, 128, 1, 1, 36, 128},
            {0, 128, 0, 1, 0, 256, 128, 1, 6, 2, 128},
            {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
        };
        for (int i = 0; i < N; i++) {
            Region src, dst;
            ::memcpy(&src, data[3 * i], 44);
            ::memcpy(&dst, data[3 * i + 1], 44);
            bool fused = TensorUtils::fuseRegion(src, dst);
            if (data[3 * i + 2][0] < 0 && !fused) {
                continue;
            }
            int cmp = ::memcmp(&dst, data[3 * i + 2], 44);
            if (!fused || (cmp != 0)) {
                return false;
            }
        }
        return true;
    }
};
MNNTestSuiteRegister(RegionFuseTest, "core/regionfuse");
