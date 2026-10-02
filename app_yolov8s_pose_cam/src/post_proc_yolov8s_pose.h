/***********************************************************************************************************************
* Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : concat.h
* Version      : 2.6.0
* Description  : RZ/V2H DRP-AI Sample Application for mmpose Detection YOLOV8S Pose with MIPI/USB Camera
***********************************************************************************************************************/

#ifndef POST_PROC_H
#define POST_PROC_H

#include "define.h"

class PostProc
{
    public:
        PostProc();
        ~PostProc();
        void init_param();
        /* box[s]: [64,G,G], cls[s]: [1,G,G], kpt[s]: [51,G,G], s = 0..2 (80/40/20)
           output_buf: [num_grid_points][num_channels] */
        void PrePost_Proc(float* const box[3], float* const cls[3], float* const kpt[3], float* output_buf);

    private:
        static inline float sigmoid(float x) { return 1.0f / (1.0f + expf(-x)); }
};

#endif
