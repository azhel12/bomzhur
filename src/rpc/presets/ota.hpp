/**
 * @file ota.hpp
 * 
 * @brief Implements ota.X RPC methods
*/

#ifndef RPC_PRESETS_OTA_H_
#define RPC_PRESETS_OTA_H_

#include "../rpc_server.h"
#include "../../utils/base64.h"

#include <zhele/common/template_utils/type_list.h>

#include <concepts>
#include <cstdint>
#include <span>

namespace bomzhur::rpc::presets {
    /**
     * @brief What the OTA preset needs from an updater
    */
    template<typename T>
    concept firmware_updater = requires(uint32_t size, uint32_t crc32, std::span<const uint8_t> chunk) {
        { T::begin(size, crc32) } -> std::same_as<bool>;
        { T::write_chunk(chunk) } -> std::same_as<bool>;
        { T::end() } -> std::same_as<bool>;
    };

    /**
     * @brief OTA RPC API pack
     * 
     * @tparam _updater Updater module
    */
    template<firmware_updater _updater>
    struct ota {
        /// @brief ota.begin request
        struct begin_request_t {
            /// @brief Whole update file size (both slots)
            uint32_t size = 0;
            /// @brief Whole update file CRC32
            uint32_t crc32 = 0;
        };
        /// @brief ota.begin handler
        static inline request_result_t begin(std::string_view params, std::span<char> response, unsigned& response_size) {
            auto request_param = bomzhur::read_json<begin_request_t>(params);

            return _updater::begin(request_param.size, request_param.crc32)
                ? request_result_t::ok
                : request_result_t::invalid_params;
        }

        /**
         * @brief ota.write handler
        */
        static inline request_result_t write(std::string_view params, std::span<char> response, unsigned& response_size) {
            auto scratch = std::span(reinterpret_cast<uint8_t*>(response.data()), response.size());
            int decoded = base64_decode(params, scratch);

            // Flash is programmed in halfwords, so chunks must decode to an even
            // number of bytes or the next chunk would land on an odd address
            if (decoded < 0 || (decoded & 1) != 0) {
                return request_result_t::invalid_params;
            }

            bool written = _updater::write_chunk(scratch.first(decoded));

            return written ? request_result_t::ok : request_result_t::internal_error;
        }

        /// @brief ota.end handler: checks the written slot against its stamped CRC32
        static inline request_result_t end(std::string_view params, std::span<char> response, unsigned& response_size) {
            return _updater::end()
                ? request_result_t::ok
                : request_result_t::internal_error;
        }

        /// @brief ota.begin method
        using begin_method = rpc_method<"ota.begin", begin>;
        /// @brief ota.write method
        using write_method = rpc_method<"ota.write", write>;
        /// @brief ota.end method
        using end_method = rpc_method<"ota.end", end>;
        /// @brief Full OTA methods pack
        using all_methods = Zhele::template_utils::type_list<begin_method, write_method, end_method>;
    };
} // namespace bomzhur::rpc::presets

#endif //! RPC_PRESETS_OTA_H_