// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** @file projection_interface.h
 *
 * Defines the Projection interface, which defines mapping between
 * pixel planes and real world, mapping between color and depth cameras with given calibration parameters.
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
#endif /* realsense_log_utils_EXPORTS */
#else /* defined (WIN32) */
#define DLL_EXPORT
#endif

extern "C" {
    void* rs_projection_create_instance_from_intrinsics_extrinsics(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics);
}

namespace rs
{
    namespace core
    {

        /**
         * @class projection_interface
         *
         * This interface defines mapping between cameras and projection to and deprojection from real world.
         * The real world coordinate system is the right-handed coordinate system.
         * The interface requires calibration data of each sensor intrinsic parameters, which describe the camera model,
         * and extrinsic parameters, which describe the transformation between two sensors coordinate systems.
         * Call rs::core::projection_interface::create_instance to create an instance of this interface.
         */
        class DLL_EXPORT projection_interface : public release_interface
        {
        public:
            /**
             * @brief map_depth_to_color maps depth image pixels to corresponding color image pixels.
             *
             * Retrieve color image pixel based on provided depth image pixel coordinates and the depth value of the pixel.
             * This method has optimized performance for a few pixels.
             * @param[in]  npoints                 The number of pixels to be mapped.
             * @param[in]  pos_uvz                 The array of depth pixel coordinates + depth value in the Point3DF32 structure.
             * @param[out] pos_ij                  The array of color pixel coordinates, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0 or depth value is less than (float)2e-38 for certain point.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status map_depth_to_color(int32_t npoints, point3dF32 *pos_uvz, pointF32 *pos_ij) = 0;

            /**
             * @brief map_color_to_depth maps color image pixels to depth image pixels.
             *
             * Retrieve depth image pixel based on provided color image pixel coordinates.
             * This method has optimized performance for a few pixels.
             * This method creates UV Map to perform the mapping.
             * @param[in]  depth                   The depthmap image.
             * @param[in]  npoints                 The number of pixels to be mapped.
             * @param[in]  pos_ij                  The array of color pixel coordinates.
             * @param[out] pos_uv                  The array of depth pixel coordinates, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0 or depth value is less than (float)2e-38 for certain point.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv) = 0;

            /**
             * @brief project_depth_to_camera deprojects depth image pixels to camera (real world) points.
             *
             * Deproject camera coordinate system (depth image) to real world coordinate system (camera) with the origin at the center of the camera sensor
             * for a number of points.
             * The real world coordinate system is the right-handed coordinate system.
             * This method has optimized performance for a few points.
             * @param[in]   npoints                The number of points to be deprojected.
             * @param[in]   pos_uvz                The array of depth pixel coordinates + depth value in the Point3DF32 structure.
             * @param[out]  pos3d                  The array of world point coordinates, in mm, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d) = 0;

            /**
             * @brief project_color_to_camera deprojects color image pixels to camera (real world) points.
             *
             * Deproject camera coordinate system (color image) to real world coordinate system (camera) with the origin at the center of the camera sensor
             * for a number of points.
             * The real world coordinate system is the right-handed coordinate system.
             * This method has optimized performance for a few points.
             * @param[in]   npoints                The number of points to be deprojected.
             * @param[in]   pos_ijz                The array of color pixel coordinates + depth value in the Point3DF32 structure.
             * @param[out]  pos3d                  The array of camera point coordinates, in mm, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d) = 0;

            /**
             * @brief project_camera_to_depth projects camera (real world) points to depth image pixels.
             *
             * Project real world coordinate system (camera) to camera coordinate system (depth image) for a number of points.
             * The real world coordinate system is expected to be the right-handed coordinate system.
             * This method has optimized performance for a few points.
             * @param[in]    npoints               The number of points to be projected.
             * @param[in]    pos3d                 The array of world point coordinates, in mm.
             * @param[out]   pos_uv                The array of depth pixel coordinates, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0 or depth value is less than (float)2e-38 for certain point.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv) = 0;

            /**
             * @brief project_camera_to_color projects camera (real world) points to corresponding color image pixels.
             *
             * Project real world coordinate system (camera) to camera coordinate system (color image) for a number of points.
             * The real world coordinate system is expected to be the right-handed coordinate system.
             * This method has optimized performance for a few points.
             * @param[in]    npoints               The number of points to be mapped.
             * @param[in]    pos3d                 The array of world point coordinates, in mm.
             * @param[out]   pos_ij                The array of color pixel coordinates, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_param_unsupported   npoints value passed equals 0 or depth value is less than (float)2e-38 for certain point.
             * @return: status_handle_invalid      Invalid [in] or [out] array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij) = 0;

            /**
             * @brief query_uvmap retrieves UV map for specific depth image.
             *
             * Retrieve the UV Map for the specific depth image. The UV Map is a PointF32 array of depth size width*height.
             * Each UV Map pixel contains the corresponding normalized color image pixel coordinates which match the depth image pixel with the same
             * image coordinates as the UV map pixel.
             * @param[in]  depth                   The depth image instance.
             * @param[out] uvmap                   The UV Map, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_handle_invalid      Invalid depth image or uvmap array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status query_uvmap(image_interface *depth, pointF32 *uvmap) = 0;

            /**
             * @brief query_invuvmap retrieves Inversed UV map for specific depth image.
             *
             * Retrieve the Inverse UV Map for the specific depth image. The Inversed UV Map is a PointF32 array of color size width*height.
             * Each Inversed UV map pixel contains the corresponding normalized depth image pixel coordinates which match the color image pixel with the same
             * image coordinates as the Inversed UV map pixel.
             * This method creates UV Map to create Inversed UV Map.
             * @param[in]  depth                   The depth image instance.
             * @param[out] inv_uvmap               The Inversed UV Map, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_handle_invalid      Invalid depth image or invuvmap array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status query_invuvmap(image_interface *depth, pointF32 *inv_uvmap) = 0;

            /**
             * @brief query_vertices retrieves 3D points array of depth resolution with units in mm.
             *
             * Retrieve the vertices for the specific depth image. The vertices is a Point3DF32 array of depth
             * size width*height. The world coordinates units are in mm.
             * The points array coordinates are in the real world coordinate system with the origin at the center of the camera sensor.
             * The real world coordinate system is the right-handed coordinate system.
             * @param[in]  depth                   The depth image instance.
             * @param[out] vertices                The 3D vertices (real world points) in real world coordinates, to be returned.
             * @return: status_no_error            Successful execution.
             * @return: status_handle_invalid      Invalid depth image or vertices array passed as parameter.
             * @return: status_data_unavailable    Incorrect depth or color data passed in projection initialization.
             */
            virtual status query_vertices(image_interface *depth, point3dF32 *vertices) = 0;

            /**
             * @brief create_color_image_mapped_to_depth maps every color pixel for every depth pixel and output image_interface instance.
             *
             * Retrieve every color pixel for every depth pixel using the UV map, and output a color image, aligned in space
             * and resolution to the depth image.
             * Returned data is wrapped in image_interface with self-releasing mechanism.
             * This method creates UV Map to perform the mapping.
             * The holes (if any) are left empty (expect the pixel value to be 0).
             * The memory is owned by the user. Wrap an instance in sdk unique_ptr to release the memory using self-releasing mechanism.
             * @param[in] depth                    The depth image instance.
             * @param[in] color                    The color image instance.
             * @return: image_interface*           The output image in the depth image resolution.
             * @return: nullptr                    Invalid depth or color image passed as parameter or uvmap failed to create.
             */
            virtual image_interface* create_color_image_mapped_to_depth(image_interface *depth, image_interface *color) = 0;

            /**
             * @brief create_depth_image_mapped_to_color maps depth pixels to color resolution and output image_interface instance.
             *
             * Map every depth pixel to the color image resolution, and output a depth image, aligned in space
             * and resolution to the color image. The color image size may be different from original.
             * Returned data is wrapped in image_interface with self-releasing mechanism.
             * This method creates UV Map to perform the mapping.
             * The holes (if any) are left empty (expect the pixel value to be 0).
             * The memory is owned by the user. Wrap an instance in sdk unique_ptr to release the memory using self-releasing mechanism.
             * @param[in] depth                    The depth image instance.
             * @param[in] color                    The color image instance.
             * @return: image_interface*           The output image in the color image resolution.
             * @return: nullptr                    Invalid depth or color image passed as parameter or uvmap failed to create.
             */
            virtual image_interface* create_depth_image_mapped_to_color(image_interface *depth, image_interface *color) = 0;


            /**
             * @brief create_instance creates projection_interface instance based on intrinsic and extrinsic parameters.
             *
             * Create instance and initilize on intrinsics and extrinsics
             * @param[in] colorIntrinsics          Camera color intrinsics.
             * @param[in] depthIntrinsics          Camera depth intrinsics.
             * @param[in] extrinsics               Camera depth to color extrinsics.
             * @return: projection_interface*      Instance of projection_interface.
             * @return: nullptr                    Provided intrisics or extrinsics are not initialized.
             */
			static projection_interface* create_instance(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics)
            {
                return (projection_interface*)rs_projection_create_instance_from_intrinsics_extrinsics(colorIntrinsics, depthIntrinsics, extrinsics);
            }
        protected:
            //force deletion using the release function
            virtual ~projection_interface() {}
        };

    }
}


