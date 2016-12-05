// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file release_self_base.h
* @brief Describes the \c rs::utils::release_self_base template.
**/

#pragma once

namespace rs
{
    namespace utils
    {
        /**
        * 
        * @brief Provides an ABI-safe release operation for a single non-ref counted object.
        *
        * Calling to release will delete the object from the context of the intializing side. With this method,
		* there is no need to supply an additional ABI-safe object deleter method.
        */
        template<typename T>
        class release_self_base : public T
        {

        public:
            /**
            * @brief Deletes the current instance.
            *
            * Releases the object from the context of the intializing side. With this method,
		    * there is no need to supply an additional ABI-safe object deleter method.
            * @return int Number of valid references for this instance
            */
            virtual int release() const override
            {
                delete(this);
                return 0;
            }
        protected:
            virtual ~release_self_base () {}
        };
    }
}
