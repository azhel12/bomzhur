/**
 * @file system.hpp
 * 
 * @brief Implements system.X RPC methods
*/

#ifndef RPC_PRESETS_SYSTEM_H_
#define RPC_PRESETS_SYSTEM_H_

#include "../rpc_server.h"

#include <zhele/common/template_utils/type_list.h>

#include <cstdint>
#include <type_traits>

namespace bomzhur::rpc::presets {
    /**
     * @brief System RPC API pack
     *
     * @tparam _deferred_reboot_cbk Schedules a reset for after the answer has
     * left the wire (argument: delay in milliseconds). Without it sys.reboot
     * answers with an error.
    */
    template<std::add_pointer_t<void(uint32_t)> _deferred_reboot_cbk = nullptr>
    struct system {
        /// @brief sys.getinfo handler: the firmware version as a JSON string
        static inline request_result_t getinfo(std::string_view params, std::span<char> response, unsigned& response_size) {
            static constexpr std::string_view version = R"(")" FW_VERSION R"(")";

            std::copy(version.begin(), version.end(), response.begin());
            response_size = version.size();

            return request_result_t::ok;
        }

        /// @brief sys.reboot handler
        static inline request_result_t reboot(std::string_view params, std::span<char> response, unsigned& response_size) {
            if constexpr (_deferred_reboot_cbk != nullptr) {
                _deferred_reboot_cbk(500);
                static constexpr std::string_view ok_response = R"("OK")";
                std::copy(std::begin(ok_response), std::end(ok_response), std::begin(response));
                response_size = ok_response.length();
                return request_result_t::ok;
            }

            return request_result_t::internal_error;
        }

        /// @brief sys.getinfo method
        using getinfo_method = rpc_method<"sys.getinfo", getinfo>;
        /// @brief sys.reboot method
        using reboot_method = rpc_method<"sys.reboot", reboot>;
        /// @brief Full system methods pack
        using all_methods = Zhele::template_utils::type_list<getinfo_method, reboot_method>;
    };
} // namespace bomzhur::rpc::presets

#endif //! RPC_PRESETS_SYSTEM_H_