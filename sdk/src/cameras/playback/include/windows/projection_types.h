// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#include "ds_calib_rect_params.h"
#include <memory>

namespace ivcam_projection
{
    typedef struct
    {
        float	Rmax;
        float	Kc[3][3];		//[3x3]: intrinsic calibration matrix of the IR camera
        float	Distc[5];		// [1x5]: forward distortion parameters of the IR camera
        float	Invdistc[5];	// [1x5]: the inverse distortion parameters of the IR camera
        float	Pp[3][4];		// [3x4] : projection matrix
        float	Kp[3][3];		// [3x3]: intrinsic calibration matrix of the projector
        float	Rp[3][3];		// [3x3]: extrinsic calibration matrix of the projector
        float	Tp[3];			// [1x3]: translation vector of the projector
        float	Distp[5];		// [1x5]: forward distortion parameters of the projector
        float	Invdistp[5];	// [1x5]: inverse distortion parameters of the projector
        float	Pt[3][4];		// [3x4]: IR to RGB (texture mapping) image transformation matrix
        float	Kt[3][3];
        float	Rt[3][3];
        float	Tt[3];
        float	Distt[5];		// [1x5]: The inverse distortion parameters of the RGB camera
        float	Invdistt[5];
        float   QV[6];
    } TCalibrationParameters;

    struct ProjectionParamsF250
    {
        int version;
        uint32_t depthWidth;
        uint32_t depthHeight;
        uint32_t colorWidth;
        uint32_t colorHeight;
        TCalibrationParameters calibrationParams;
    };

    typedef enum
    {
        LeftHandedCoordinateSystem  =0,
        RightHandedCoordinateSystem
    } TCoordinatSystemDirection;

    typedef enum
    {
        UnMirroredCamera = 0 ,// For world facing cameras
        MirroredCamera   = 1  // For user facing cameras
    } TMirroredCamera;

    struct ProjectionHeaderF250
    {
        char verHeader[8];
        uint32_t Version;
        uint32_t VersionMinor;
        TCoordinatSystemDirection CoordinatSystemDirection;
        TMirroredCamera mirrorMode;
    };
}

namespace ds_projection
{

    // Do not change
    enum ProjectionDataVersions
    {
        VERSION_0 = 1,
        VERSION_1 = 2
    };
    enum DsapiProjectionCalibrationVersions
    {
        ALPHA = 0,
        BETA = 1,
        BETA2 = 2
    };

    typedef struct _ColorStreamParameters
    {
        DSCalibIntrinsicsRectified calibIntrinsicsRectified;
        DSCalibIntrinsicsNonRectified calibIntrinsicsNonRectified;
//#if defined(__ANDROID__)
//		char __reserved[4]; /// To match Windows alignment
//#endif
        double zToRectColorTranslation[3];
        double zToNonRectColorTranslation[3];
        double rotation[9];
        double rectColorToNonRectColorRotation[9];
        bool isRectified;
        int width;
        int height;
//#if defined(__ANDROID__) && !defined(__x86_64)
//        char __reserved2[4]; /// To match Windows alignment
//#endif
    } ColorStreamParameters;

    struct ProjectionData
    {
        // any change to this struct must increment the VERSION value
        static const unsigned int VERSION = ProjectionDataVersions::VERSION_1;

        unsigned int version;
        unsigned int calibParamsSize;
        DSCalibRectParameters calibParams;
        DSCalibIntrinsicsRectified zIntrinRect;
        DSCalibIntrinsicsRectified lrIntrinRect;
        DSCalibIntrinsicsNonRectified zIntrinNonRect;
        DSCalibIntrinsicsNonRectified lIntrinNonRect;
        DSCalibIntrinsicsNonRectified rIntrinNonRect;
        int dWidth, dHeight;
        bool zRectified;
        bool isMirrored;

        ColorStreamParameters platformCameraParams;
        ColorStreamParameters thirdCameraParams;
    };

    struct Version0_ProjectionData
    {
        unsigned int version;
        unsigned int calibParamsSize;
        DSCalibRectParameters calibParams;
        DSCalibIntrinsicsRectified zIntrinRect;
        DSCalibIntrinsicsRectified lrIntrinRect;
        DSCalibIntrinsicsRectified thirdIntrinRect;
        DSCalibIntrinsicsNonRectified zIntrinNonRect;
        DSCalibIntrinsicsNonRectified lIntrinNonRect;
        DSCalibIntrinsicsNonRectified rIntrinNonRect;
        DSCalibIntrinsicsNonRectified thirdIntrinNonRect;
        double zToThirdTranslation[3];
        double rotation[9];
        int cWidth, cHeight;
        int dWidth, dHeight;
        bool zRectified;
        bool thirdRectified;
        bool isMirrored;

        operator ProjectionData() const
        {
            ProjectionData retVal = { 0 };

            retVal.version = version;
            retVal.calibParamsSize = calibParamsSize;
            retVal.calibParams = calibParams;
            retVal.zIntrinRect = zIntrinRect;
            retVal.lrIntrinRect = lrIntrinRect;
            retVal.zIntrinNonRect = zIntrinNonRect;
            retVal.lIntrinNonRect = lIntrinNonRect;
            retVal.rIntrinNonRect = rIntrinNonRect;
            retVal.dWidth = dWidth;
            retVal.dHeight = dHeight;
            retVal.zRectified = zRectified;
            retVal.isMirrored = isMirrored;

            retVal.thirdCameraParams.height = cHeight;
            retVal.thirdCameraParams.width = cWidth;
            retVal.thirdCameraParams.isRectified = thirdRectified;
            memcpy(retVal.thirdCameraParams.rotation, rotation, sizeof(double) * 9);
            memcpy(retVal.thirdCameraParams.zToNonRectColorTranslation, zToThirdTranslation, sizeof(double) * 3);
            memcpy(retVal.thirdCameraParams.zToRectColorTranslation, zToThirdTranslation, sizeof(double) * 3);
            retVal.thirdCameraParams.calibIntrinsicsRectified = thirdIntrinRect;
            retVal.thirdCameraParams.calibIntrinsicsNonRectified = thirdIntrinNonRect;

            return retVal;
        }
    };

}


