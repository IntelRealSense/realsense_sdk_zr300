// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file projection_interface.h
* @brief
* Describes the \c rs::core::projection_interface class.
* 
* Defines mapping between pixel planes and real world, mapping between color and depth cameras with given calibration parameters.
*/

#pragma once
#include "rs/core/status.h"
#include "rs/core/types.h"
#include "rs/core/image_interface.h"

#ifdef WIN32 
#ifdef realsense_projection_EXPORTS
#define  DLL_EXPORT __declspec(dllexport)
#else
#define  DLL_EXPORT __declspec(dllimport)
#endif /* realsense_projection_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

DLL_EXPORT extern "C" {
	void*  rs_projection_create_instance_from_intrinsics_extrinsics(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics);
}

namespace rs
{
    namespace core
    {

        /**
		* \brief
        * Defines mapping between cameras and projection to and unprojection from real world.
        *
		* The real world coordinate system is the right-handed coordinate system.
        * The interface requires calibration data of each sensor intrinsic parameters, which describe the camera model,
        * and extrinsic parameters, which describe the transformation between two sensors coordinate systems.
		*
		* Call the \c rs::core::projection::create_instance() method
        * to create an instance of this interface.
        */
        class DLL_EXPORT projection_interface : public release_interface
        {
        public:
            /**
            * @brief Maps depth coordinates to color coordinates for a few pixels.
            *
            * Retrieve color coordinates based on provided depth coordinates and the depth value of the pixel.
            * This method has optimized performance for a few pixels.
            * @param[in]  npoints                 Number of pixels to be mapped
            * @param[in]  pos_uvz                 Array of depth coordinates and depth value in the \c Point3DF32 structure
            * @param[out] pos_ij                  Array of color coordinates to be returned
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    \c npoints value passed equals 0 or depth value is less than <tt>(float)2e-38 </tt> for a certain point.
            * @return status_handle_invalid       Invalid in or out array passed as parameter
            * @return status_data_unavailable     Incorrect depth or color data passed in projection initialization
            */
            virtual status map_depth_to_color(int32_t npoints, point3dF32  *pos_uvz, pointF32  *pos_ij) = 0;

            /**
            * @brief Maps color coordinates to depth coordinates for a few pixels.
            *
            * Retrieve depth coordinates based on provided color coordinates.
            * This method has optimized performance for a few pixels.
            * This method creates UV Map to perform the mapping.
            * @param[in]  depth           Depth map image
            * @param[in]  npoints         Number of pixels to be mapped
            * @param[in]  pos_ij          Array of color coordinates
            * @param[out] pos_uv          Array of depth coordinates to be returned
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    \c npoints value passed equals 0 or depth value is less than <tt>(float)2e-38 </tt> for a certain point.
            * @return status_handle_invalid       Invalid in or out array passed as parameter
            * @return status_data_unavailable     Incorrect depth or color data passed in projection initialization
            */
            virtual status map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv) = 0;

            /**
            * @brief Maps depth coordinates to world coordinates for a few pixels.
            *
            * Deproject camera coordinate system (depth image) to real world coordinate system (camera) with the origin at the center of the camera sensor
            * for a number of points.
            * The real world coordinate system is the right-handed coordinate system.
            * This method has optimized performance for a few points.
            * @param[in]   npoints                Number of points to be deprojected.
            * @param[in]   pos_uvz                Array of depth pixel coordinates and depth value in the \c Point3DF32 structure.
            * @param[out]  pos3d                  Array of world point coordinates to be returned, in millimeters
            * @return status_no_error            Successful execution.
            * @return status_param_unsupported   \c npoints value passed equals 0
            * @return status_handle_invalid      Invalid in or out array passed as parameter
            * @return status_data_unavailable    Incorrect depth or color data passed in projection initialization.
            */
            virtual status project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d) = 0;

            /**
            * @brief Deprojects color image pixels to camera (real world) points.
            *
            * Deproject camera coordinate system (color image) to real world coordinate system (camera) with the origin at the center of the camera sensor
            * for a number of points.
            * The real world coordinate system is the right-handed coordinate system.
            * This method has optimized performance for a few points.
            * @param[in]   npoints               Number of points to be deprojected
            * @param[in]   pos_ijz               Array of color pixel coordinates and depth value in the \c Point3DF32 structure.
            * @param[out]  pos3d                 Array of camera point coordinates to be returned, in millimeters
            * @return status_no_error            Successful execution
            * @return status_param_unsupported   \c npoints value passed equals 0
            * @return status_handle_invalid      Invalid in or out array passed as parameter
            * @return status_data_unavailable    Incorrect depth or color data passed in projection initialization
            */
            virtual status project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d) = 0;

            /**
            * @brief Projects camera (real world) points to depth image pixels.
            *
            * Project real world coordinate system (camera) to camera coordinate system (depth image) for a number of points.
            * The real world coordinate system is expected to be the right-handed coordinate system.
            * This method has optimized performance for a few points.
            * @param[in]    npoints               Number of points to be projected
            * @param[in]    pos3d                 Array of world point coordinates, in millimeters
            * @param[out]   pos_uv                Array of depth pixel coordinates to be returned
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    \c npoints value passed equals 0 or depth value is less than <tt>(float)2e-38 </tt> for a certain point.
            * @return status_handle_invalid       Invalid in or out array passed as parameter
            * @return status_data_unavailable     Incorrect depth or color data passed in projection initialization
            */
            virtual status project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv) = 0;

            /**
            * @brief Projects camera (real world) points to corresponding color image pixels.
            *
            * Project real world coordinate system (camera) to camera coordinate system (color image) for a number of points.
            * The real world coordinate system is expected to be the right-handed coordinate system.
            * This method has optimized performance for a few points.
            * @param[in]    npoints               Number of points to be mapped
            * @param[in]    pos3d                 Array of world point coordinates, in millimeters
            * @param[out]   pos_ij                Array of color pixel coordinates, to be returned
            * @return status_no_error             Successful execution
            * @return status_param_unsupported    \c npoints value passed equals 0 or depth value is less than <tt>(float)2e-38 </tt> for a certain point.
            * @return status_handle_invalid       Invalid in or out array passed as parameter
            * @return status_data_unavailable     Incorrect depth or color data passed in projection initialization
            */
            virtual status project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij) = 0;

            /**
            * @brief Retrieves UV map for specific depth image.
            *
            * Retrieve the UV Map for the specific depth image. The UV Map is a \c PointF32 array of depth size \c width*height.
            * Each UV Map pixel contains the corresponding normalized color image pixel coordinates which match the depth image pixel with the same
            * image coordinates as the UV map pixel.
            * @param[in]  depth                  Depth image instance
            * @param[out] uvmap                  UV map, to be returned
            * @return status_no_error            Successful execution
            * @return status_handle_invalid      Invalid depth image or uvmap array passed as parameter
            * @return status_data_unavailable    Incorrect depth or color data passed in projection initialization
            */
            virtual status query_uvmap(image_interface *depth, pointF32 *uvmap) = 0;

            /**
            * @brief Retrieves Inversed UV map for specific depth image.
            *
            * The Inversed UV Map is a \c PointF32 array of color size \c width*height.
            * Each Inversed UV map pixel contains the corresponding normalized depth image pixel coordinates which match the color image pixel with the same
            * image coordinates as the Inversed UV map pixel.
            * @param[in]  depth                  Depth image instance
            * @param[out] inv_uvmap              Inversed UV Map, to be returned
            * @return status_no_error            Successful execution
            * @return status_handle_invalid      Invalid depth image or invuvmap array passed as parameter
            * @return status_data_unavailable    Incorrect depth or color data passed in projection initialization
            */
            virtual status query_invuvmap(image_interface *depth, pointF32 *inv_uvmap) = 0;

            /**
            * @brief Retrieves 3D points array of depth resolution with units in millimeters.
            *
            * Retrieve the vertices for the specific depth image. The vertices is a \c Point3DF32 array of depth
            * size \c width*height. The world coordinates units are in millimeters.
            * The points array coordinates are in the real world coordinate system with the origin at the center of the camera sensor.
            * The real world coordinate system is the right-handed coordinate system.
            * @param[in]  depth                   Depth image instance
            * @param[out] vertices                3D vertices (real world points) to be returned, in real world coordinates
            * @return status_no_error             Successful execution
            * @return status_handle_invalid       Invalid depth image or vertices array passed as parameter
            * @return status_data_unavailable     Incorrect depth or color data passed in projection initialization
            */
            virtual status query_vertices(image_interface *depth, point3dF32 *vertices) = 0;

            /**
            * @brief Maps every color pixel for every depth pixel and output \c image_interface instance.
            *
            * Retrieves every color pixel for every depth pixel using the UV map, and outputs a color image, aligned in space
            * and resolution to the depth image.
			*
            * Returned data is wrapped in \c image_interface with self-releasing mechanism.
            * This method creates a UV Map to perform the mapping.
            * The holes (if any) are left empty (expect the pixel value to be 0).
            * The memory is owned by the user. Wrap an instance in the SDK \c unique_ptr to release the memory using a self-releasing mechanism.
            * @param[in] depth        Depth image instance
            * @param[in] color        Color image instance
            * @return image_interface*     Output image in the depth image resolution
            * @return nullptr              Invalid depth or color image passed as a parameter or uvmap failed to create.
            */
            virtual image_interface* create_color_image_mapped_to_depth(image_interface *depth, image_interface *color) = 0;

            /**
            * @brief Maps every depth pixel to the color image resolution and outputs a depth image, aligned in space 
			* and resolution to the color image.
			*
			* The color image size may be different from original.
            * Returned data is wrapped in \c image_interface with self-releasing mechanism.
            * This method creates UV Map to perform the mapping.
            * The holes (if any) are left empty (expect the pixel value to be 0).
            * The memory is owned by the user. Wrap an instance in sdk \c unique_ptr to release the memory using self-releasing mechanism.
            * @param[in] depth                   Depth image instance
            * @param[in] color                   Color image instance
            * @return image_interface*           Output image in the color image resolution
            * @return nullptr                    Invalid depth or color image passed as parameter or uvmap failed to create
            */
            virtual image_interface* create_depth_image_mapped_to_color(image_interface *depth, image_interface *color) = 0;


             /**
             * @brief Creates an instance and initializes, based on intrinsic and extrinsic parameters.
             *
             * @param[in] colorIntrinsics         Camera color intrinsics
             * @param[in] depthIntrinsics         Camera depth intrinsics
             * @param[in] extrinsics              Camera depth to color extrinsics
             * @return projection_interface*      Instance of projection_interface
             * @return nullptr                    Provided intrisics or extrinsics are not initialized
             */
			static __inline projection_interface* create_instance(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics)
            {
                return (projection_interface*)rs_projection_create_instance_from_intrinsics_extrinsics(colorIntrinsics, depthIntrinsics, extrinsics);
            }
        protected:
            //force deletion using the release function
            virtual ~projection_interface() {}
        };

    }
}


