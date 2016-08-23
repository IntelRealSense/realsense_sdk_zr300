// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** @file projection.h
    Defines the Projection interface, which defines mappings between
    pixel, depth, and real world coordinates.
  */
#pragma once
#include "rs/core/status.h"
#include "rs/core/types.h"
#include "rs/core/image_interface.h"

extern "C" {
    void* rs_projection_create_instance_from_intrinsics_extrinsics(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics);
}

namespace rs
{
    namespace core
    {

        /**
        This interface defines mappings between various coordinate systems
        used by modules of the SDK. Call the rs::core::projection::create_instance
        to create an instance of this interface.
        */
        class projection_interface
        {
        public:
            virtual ~projection_interface() {}
            /**
            @brief Map depth coordinates to color coordinates for a few pixels.
            @param[in]  npoints         The number of pixels to be mapped.
            @param[in]  pos_uvz         The array of depth coordinates + depth value in the Point3DF32 structure.
            @param[out] pos_ij          The array of color coordinates, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */
            virtual status map_depth_to_color(int32_t npoints, point3dF32  *pos_uvz, pointF32  *pos_ij) = 0;

            /**
            @brief Map color coordinates to depth coordiantes for a few pixels.
            @param[in]  depth           The depthmap image.
            @param[in]  npoints         The number of pixels to be mapped.
            @param[in]  pos_ij          The array of color coordinates.
            @param[out] pos_uv          The array of depth coordinates, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */
            virtual status map_color_to_depth(image_interface *depth, int32_t npoints, pointF32 *pos_ij, pointF32 *pos_uv) = 0;

            /**
            @brief Map depth coordinates to world coordinates for a few pixels.
            @param[in]   npoints        The number of pixels to be mapped.
            @param[in]   pos_uvz        The array of depth coordinates + depth value in the Point3DF32 structure.
            @param[out]  pos3d          The array of world coordinates, in mm, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */
            virtual status project_depth_to_camera(int32_t npoints, point3dF32 *pos_uvz, point3dF32 *pos3d) = 0;

            /**
            @brief Map color pixel coordinates to camera coordinates for a few pixels.
            @param[in]   npoints        The number of pixels to be mapped.
            @param[in]   pos_ijz        The array of color coordinates + depth value in the Point3DF32 structure.
            @param[out]  pos3d          The array of camera coordinates, in mm, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */

            virtual status project_color_to_camera(int32_t npoints, point3dF32 *pos_ijz, point3dF32 *pos3d) = 0;

            /**
            @brief Map camera coordinates to depth coordinates for a few pixels.
            @param[in]    npoints       The number of pixels to be mapped.
            @param[in]    pos3d         The array of world coordinates, in mm.
            @param[out]   pos_uv        The array of depth coordinates, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */
            virtual status project_camera_to_depth(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_uv) = 0;

            /**
            @brief Map camera coordinates to color coordinates for a few pixels.
            @param[in]    npoints       The number of pixels to be mapped.
            @param[in]    pos3d         The array of world coordinates, in mm.
            @param[out]   pos_ij        The array of color coordinates, to be returned.
            @return STATUS_NO_ERROR     Successful execution.
            */
            virtual status project_camera_to_color(int32_t npoints, point3dF32 *pos3d, pointF32 *pos_ij) = 0;

            /**
            @brief Retrieve the UV map for the specific depth image. The UVMap is a PointF32 array of depth size width*height.
            @param[in]  depth        The depth image instance.
            @param[out] uvmap        The UV map, to be returned.
            @return STATUS_NO_ERROR  Successful execution.
            */
            virtual status query_uvmap(image_interface *depth, pointF32 *uvmap) = 0;

            /**
            @brief Retrieve the inveRSe UV map for the specific depth image. The inveRSe UV map maps color coordinates
            back to the depth coordinates. The inveRSe UVMap is a PointF32 array of color size width*height.
            @param[in]  depth        The depth image instance.
            @param[out] inv_uvmap    The inveRSe UV map, to be returned.
            @return STATUS_NO_ERROR  Successful execution.
            */
            virtual status query_invuvmap(image_interface *depth, pointF32 *inv_uvmap) = 0;

            /**
            @brief Retrieve the vertices for the specific depth image. The vertices is a Point3DF32 array of depth
            size width*height. The world coordiantes units are in mm.
            @param[in]  depth        The depth image instance.
            @param[out] vertices     The 3D vertices in World coordinates, to be returned.
            @return STATUS_NO_ERROR Successful execution.
            */
            virtual status query_vertices(image_interface *depth, point3dF32 *vertices) = 0;

            /**
            @brief Get the color pixel for every depth pixel using the UV map, and output a color image, aligned in space
            and resolution to the depth image.
            @param[in] depth        The depth image instance.
            @param[in] color        The color image instance.
            @return The output image in the depth image resolution.
            */
            virtual image_interface* create_color_image_mapped_to_depth(image_interface *depth, image_interface *color) = 0;

            /**
            @brief Map every depth pixel to the color image resolution, and output a depth image, aligned in space
            and resolution to the color image. The color image size may be different from original.
            @param[in] depth        The depth image instance.
            @param[in] color        The color image instance.
            @return The output image in the color image resolution.
            */
            virtual image_interface* create_depth_image_mapped_to_color(image_interface *depth, image_interface *color) = 0;


            /**
            @brief Create instance and initilize on intrinsics and extrinsics
            */
            static __inline projection_interface* create_instance(rs::core::intrinsics *colorIntrinsics, rs::core::intrinsics *depthIntrinsics, rs::core::extrinsics *extrinsics)
            {
                return (projection_interface*)rs_projection_create_instance_from_intrinsics_extrinsics(colorIntrinsics, depthIntrinsics, extrinsics);
            }

        };

    }
}


