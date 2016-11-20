// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

/** @file status.h
    Defines the return status codes used by SDK interfaces.
  */
#pragma once

namespace rs
{
    namespace core
    {
        /**
            This enumeration defines various return codes that SDK interfaces
            use.  Negative values indicate errors, a zero value indicates success,
            and positive values indicate warnings.
        */
        enum status
        {
            /* success */
            status_no_error = 0,                /**< Indicates the operation succeeded without any warning */

            /* errors */
            status_feature_unsupported = -1,    /**< Unsupported feature */
            status_param_unsupported = -2,      /**< Unsupported parameter(s) */
            status_item_unavailable = -3,       /**< Item not found/not available */
            status_key_already_exists = -4,     /**< The key already exists in the data structure */
            status_invalid_argument = -5,       /**< The argument passed to the function is invalid */

            status_handle_invalid = -101,       /**< Invalid session, algorithm instance, or pointer */
            status_alloc_failed = -102,         /**< Memory allocation failure */

            status_device_failed = -201,        /**< device failed due to malfunctioning */
            status_device_lost = -202,          /**< device failed due to unplug or unavailability */

            status_exec_aborted = -301,         /**< Execution aborted due to errors in upstream components */
            status_exec_inprogress = -302,      /**< Asynchronous operation is in execution */
            status_exec_timeout = -303,         /**< Operation time out */

            status_file_write_failed = -401,    /**< Failure in open file in WRITE mode */
            status_file_read_failed = -402,     /**< Failure in open file in READ mode */
            status_file_close_failed = -403,    /**< Failure in close a file handle */
            status_file_open_failed = -404,     /**< Failure in open a file handle */

            status_data_unavailable = -501,     /**< Data not available for MW model or processing */

            status_data_not_initialized = -502, /**< Data failed to initialize */
            status_init_failed = -503,          /**< Module failure during initialization */

            status_match_not_found = -601,      /**< Matching frame not found */

            status_invalid_state = -701,        /**< current state does not allow this operation */

            /* warnings */
            status_time_gap = 101,              /**< time gap in time stamps */
            status_param_inplace = 102,         /**< the same parameters already defined */
            status_data_not_changed = 103,      /**< Data not changed (no new data available)*/
            status_process_failed = 104,        /**< Module failure during processing */
            status_value_out_of_range = 105,    /**< Data value(s) out of range*/
            status_data_pending = 106,          /**< Not all data was copied, more data is available for fetching*/
        };
    }
}
