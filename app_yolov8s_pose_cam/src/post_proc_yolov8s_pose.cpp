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

void PostProc::PrePost_Proc(float* const box[3], float* const cls[3], float* const kpt[3], std::vector<pose_detection>& det_out)
{
    const float scale_w = (float)DRPAI_IN_WIDTH  / (float)MODEL_IN_W;
    const float scale_h = (float)DRPAI_IN_HEIGHT / (float)MODEL_IN_H;
    det_out.clear();

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
            for (int gx = 0; gx < G; gx++)
            {
                const int p = gy * G + gx;

                /* 先用 logit 比較，過濾掉絕大多數格子，省下 expf */
                if (c[p] <= TH_PROB_LOGIT) continue;
                const float score = sigmoid(c[p]);

                /* DFL: softmax over 16 bins -> expectation, order = l, t, r, b (grid units) */
                float d4[4];
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
                    d4[side] = val / sum;
                }

                const float ax = gx + 0.5f;
                const float ay = gy + 0.5f;
                const float x1 = (ax - d4[0]) * stride;
                const float y1 = (ay - d4[1]) * stride;
                const float x2 = (ax + d4[2]) * stride;
                const float y2 = (ay + d4[3]) * stride;

                pose_detection d;
                d.bbox.x = (x1 + x2) * 0.5f * scale_w;
                d.bbox.y = (y1 + y2) * 0.5f * scale_h;
                d.bbox.w = (x2 - x1) * scale_w;
                d.bbox.h = (y2 - y1) * scale_h;
                d.prob = score;
                d.kpts.resize(NUM_KPTS);
                for (int j = 0; j < NUM_KPTS; j++)
                {
                    d.kpts[j].x = (k[(j*3+0)*HW + p] * 2.0f + gx) * stride * scale_w;
                    d.kpts[j].y = (k[(j*3+1)*HW + p] * 2.0f + gy) * stride * scale_h;
                    d.kpts[j].c = sigmoid(k[(j*3+2)*HW + p]);
                }
                det_out.push_back(std::move(d));
            }
        }
    }
}
