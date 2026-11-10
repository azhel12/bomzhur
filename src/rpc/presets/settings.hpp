/**
 * @file settings.hpp
 * 
 * @brief Implements settings.X RPC methods
*/

#ifndef RPC_PRESETS_SETTINGS_H_
#define RPC_PRESETS_SETTINGS_H_

#include "../rpc_server.h"
#include "../../json/json.h"

#include <zhele/common/template_utils/type_list.h>

#include <reflect/reflect>

#include <concepts>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>

namespace bomzhur::rpc::presets {
    /**
     * @brief What the settings preset needs from a settings module
    */
    template<typename T>
    concept settings_module = requires(std::string_view section, std::string_view data, char* where, std::span<char> scratch) {
        typename T::data_t;
        { T::dump_section_to_json(section, where) } -> std::same_as<char*>;
        { T::set_from_json(section, data, scratch) } -> std::same_as<bool>;
    };

    /**
     * @brief Settings RPC API pack
     *
     * @tparam _settings Settings module
    */
    template<settings_module _settings>
    struct settings {
        /**
         * @brief Tokens of a settings.set request around the section values
        */
        static constexpr unsigned set_request_envelope_tokens = 12;

        /// @brief Largest section, in tokens
        static constexpr unsigned max_section_tokens = []<std::size_t... _index>(std::index_sequence<_index...>) {
            return std::max({0u, json_token_count<decltype(reflect::get<_index>(std::declval<typename _settings::data_t&>()))>()...});
        }(std::make_index_sequence<reflect::size<typename _settings::data_t>()>{});

        static_assert(set_request_envelope_tokens + max_section_tokens <= max_json_tokens,
            "A settings section is too big for settings.set to parse: split it or raise max_json_tokens");

        /// @brief settings.get request
        struct get_request_t {
            std::string_view section;
        };
        /// @brief settings.get handler: the section as a JSON object
        static inline request_result_t get(std::string_view params, std::span<char> response, unsigned& response_size) {
            auto request = read_json<get_request_t>(params);

            auto end = _settings::dump_section_to_json(request.section, response.data());
            if (end == nullptr)
                return request_result_t::invalid_params;

            response_size = end - response.data();

            return request_result_t::ok;
        }

        /// @brief settings.set request
        struct set_request_t {
            std::string_view section;
            std::string_view data;
        };
        /// @brief settings.set handler: merges the given values into a section and persists everything
        static inline request_result_t set(std::string_view params, std::span<char> response, unsigned& response_size) {
            auto request = read_json<set_request_t>(params);

            // The response buffer doubles as the scratch for the settings dump:
            // settings.set has no result body, so it is idle
            return _settings::set_from_json(request.section, request.data, response)
                ? request_result_t::ok
                : request_result_t::invalid_params;
        }

        /// @brief settings.get method
        using get_method = rpc_method<"settings.get", get>;
        /// @brief settings.set method
        using set_method = rpc_method<"settings.set", set>;
        /// @brief Full settings methods pack
        using all_methods = Zhele::template_utils::type_list<get_method, set_method>;
    };
} // namespace bomzhur::rpc::presets

#endif //! RPC_PRESETS_SETTINGS_H_
