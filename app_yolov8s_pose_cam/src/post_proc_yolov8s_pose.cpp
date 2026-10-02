/***********************************************************************************************************************
* Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : concat_proc.cpp
* Version      : 2.6.0
* Description  : RZ/V2H DRP-AI Sample Application for mmpose Detection YOLOV8S Pose with MIPI/USB Camera
***********************************************************************************************************************/

/*****************************************
* Includes
******************************************/
#include "post_proc_yolov8s_pose.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <algorithm>

using namespace std;

PostProc::PostProc()
{

}

PostProc::~PostProc()
{

}

void PostProc::init_param() {}

void PostProc::PrePost_Proc(float* const box[3], float* const cls[3], float* const kpt[3], float* output_buf)
{
    /* 沒過閾值的格子 conf = 0，R_Post_Proc 會直接略過 */
    memset(output_buf, 0, sizeof(float) * num_inf_out);

    uint32_t row = 0;
    for (int s = 0; s < NUM_INF_OUT_LAYER; s++)
    {
        const int G = num_grids[s];
        const int HW = G * G;
        const float stride = (float)strides[s];
        const float* b = box[s];
        const float* c = cls[s];
        const float* k = kpt[s];

        for (int gy = 0; gy < G; gy++)
        {
            for (int gx = 0; gx < G; gx++, row++)
            {
                const int p = gy * G + gx;
                const float score = sigmoid(c[p]);
                if (score <= TH_PROB) continue;

                float* out = output_buf + (size_t)row * num_channels;

                /* DFL: softmax over 16 bins -> expectation, order = l, t, r, b (in grid units) */
                float d[4];
                for (int side = 0; side < 4; side++)
                {
                    float v[DFL_BINS];
                    float maxv = -FLT_MAX;
                    for (int i = 0; i < DFL_BINS; i++)
                    {
                        v[i] = b[(side * DFL_BINS + i) * HW + p];
                        maxv = max(maxv, v[i]);
                    }
                    float sum = 0.0f, val = 0.0f;
                    for (int i = 0; i < DFL_BINS; i++)
                    {
                        float e = expf(v[i] - maxv);
                        sum += e;
                        val += e * i;
                    }
                    d[side] = val / sum;
                }

                const float ax = gx + 0.5f;
                const float ay = gy + 0.5f;
                const float x1 = (ax - d[0]) * stride;
                const float y1 = (ay - d[1]) * stride;
                const float x2 = (ax + d[2]) * stride;
                const float y2 = (ay + d[3]) * stride;

                /* center x, y, w, h (640x640 model input coordinates) */
                out[0] = (x1 + x2) * 0.5f;
                out[1] = (y1 + y2) * 0.5f;
                out[2] = x2 - x1;
                out[3] = y2 - y1;
                out[4] = score;

                /* Keypoints: x = (kx*2 + gx)*stride, y = (ky*2 + gy)*stride, conf = sigmoid */
                for (int j = 0; j < NUM_KPTS; j++)
                {
                    const float kx = k[(j * 3 + 0) * HW + p];
                    const float ky = k[(j * 3 + 1) * HW + p];
                    const float kc = k[(j * 3 + 2) * HW + p];
                    out[5 + j * 3 + 0] = (kx * 2.0f + gx) * stride;
                    out[5 + j * 3 + 1] = (ky * 2.0f + gy) * stride;
                    out[5 + j * 3 + 2] = sigmoid(kc);
                }
            }
        }
    }
}
