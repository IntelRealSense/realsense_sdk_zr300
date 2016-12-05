// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** 
* \file self_releasing_array_data_releaser.h
* @brief Describes the \c rs::utils::self_releasing_array_data_releaser class.
*/
  
#pragma once
#include "rs/utils/release_self_base.h"
#include "rs/core/release_interface.h"

namespace rs
{
    namespace utils
    {
        /**
         * 
         * @brief Array deallocation memory management class.
         *
         * When release is called, it deletes the array of data provided in the constructor and itself using the
         * \c release_self_base class.
         */
        class self_releasing_array_data_releaser : public release_self_base<rs::core::release_interface>
        {
        /*
        * @brief Osnat
		* You have "@brief Constructor" - need more.
        */
		public:
            /**
             * @brief Constructor 
             * @param data Allocated data pointer
             */
            self_releasing_array_data_releaser(uint8_t* data) :data(data) {}

            /*
            * @brief Osnat
		    * what do you mean by: "Assumes the data....?"
            */
			/**
             * @brief 
             * Assumes the data provided needs to be released with <tt>operator delete []</tt>
             * @return int Number of instances
             */
            int release() const override
            {
                if(data)
                {
                    delete [] data;
                }
                return release_self_base::release(); //object destructed, return immediately
            }
        protected:
            ~self_releasing_array_data_releaser() {}
        private:
            uint8_t* data;
        };
    }
}
