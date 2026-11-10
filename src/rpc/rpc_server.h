/**
 * @file rpc_server.h
 * 
 * @brief Implements a JSON-RPC 2.0 server over a binary RS485 frame
*/

#ifndef RPC_RPC_SERVER_H_
#define RPC_RPC_SERVER_H_

#include <reflect/reflect>

#include <zhele/common/template_utils/type_list.h>

#include <cstdint>
#include <span>
#include <string_view>

namespace bomzhur::rpc {
    /**
     * @brief RPC request result code
    */
    enum class request_result_t : int32_t {
        ok               = 0,
        parse_error      = -32700,
        invalid_request  = -32600,
        method_not_found = -32601,
        invalid_params   = -32602,
        internal_error   = -32603,
    };

    /**
     * @brief Canonical message for an error code, as the spec spells it
     *
     * @param [in] code Error code
     *
     * @returns Message text
    */
    inline constexpr std::string_view result_message(request_result_t code) {
        switch (code) {
            case request_result_t::invalid_request:
                return "Invalid Request";
            case request_result_t::method_not_found:
                return "Method not found";
            case request_result_t::invalid_params:
                return "Invalid params";
            default:
                return "Internal error";
        }
    }

    /**
     * @brief RPC method handler type
    */
    using rpc_method_handler_t = std::add_pointer_t<request_result_t(std::string_view params, std::span<char> response, unsigned& response_size)>;

    /**
     * @brief RPC method unit type (for server template instance)
     * 
     * @tparam _method_name Method name
     * @tparam _method_handler Method handler
    */
    template<reflect::fixed_string _method_name, rpc_method_handler_t _method_handler>
    struct rpc_method {
        static constexpr std::string_view name = _method_name;
        static constexpr auto handler = _method_handler;
    };

    /// @brief Trait: is the type a Zhele type_list (a preset's method pack) or a single method
    template<typename>
    struct is_type_list : std::false_type {};
    template<typename... Ts>
    struct is_type_list<Zhele::template_utils::type_list<Ts...>> : std::true_type {};

    /**
     * @brief Implements RPC server
     *
     * @tparam _device_address Device address
     * @tparam _methods Supported methods pack
    */
    template<uint8_t _device_address, typename... _methods>
    class rpc_server {
        /// @brief Build type list for given methods (supported methods and lists)
        using methods_list = decltype((std::conditional_t<is_type_list<_methods>::value, _methods, Zhele::template_utils::type_list<_methods>>{} + ...));

        /// @brief Check that no methods with same name
        static_assert(methods_list::is_unique([](auto v1, auto v2) {
            return Zhele::template_utils::type_unbox<v1>::name == Zhele::template_utils::type_unbox<v2>::name;
        }), "Two handler for one method name!!!");

        /// @brief Envelope key: method name
        static constexpr std::string_view _method_key = "method";
        /// @brief Envelope key: method params
        static constexpr std::string_view _params_key = "params";
        /// @brief Envelope key: request id
        static constexpr std::string_view _id_key = "id";

        /// @brief Prefix a handler writes its result behind
        static constexpr std::string_view _result_prefix = R"({"jsonrpc":"2.0","result":)";
        /// @brief Error envelope, assembled from scratch (a handler's output is discarded)
        static constexpr std::string_view _error_prefix = R"({"jsonrpc":"2.0","error":{"code":)";
        /// @brief Between the error code and its message
        static constexpr std::string_view _error_message_infix = R"(,"message":")";
        /// @brief Closes the error object
        static constexpr std::string_view _error_suffix = R"("})";

        /// @brief Tail appended to every response, before the id and closing brace
        static constexpr std::string_view _id_infix = R"(,"id":)";

        /// @brief Longest id echoed back: a uint32 in decimal
        static constexpr unsigned _max_id_size = 10;

        /// @brief Room kept after the result for ,"id":<id>}
        static constexpr unsigned _answer_tail_size = _id_infix.size() + _max_id_size + 1;

        /// @brief Result stand-in for methods without a response body
        static constexpr std::string_view _null_result = "null";

        /// @brief RPC frame header type
        struct rpc_frame_header {
            uint8_t dst_address;
            uint8_t src_address;
            uint8_t frame_size[2];
            uint8_t crc32[4];
            uint8_t payload_data[];
        } __packed;


    public:
        /**
         * @brief Room a handler gets for its result (and as scratch)
         *
         * @param [in] tx_buffer_size Size of the buffer handed to handle_rx()
         *
         * @returns Size of the response span every handler receives
        */
        static constexpr unsigned result_capacity(unsigned tx_buffer_size) {
            return tx_buffer_size - sizeof(rpc_frame_header) - _result_prefix.size() - _answer_tail_size;
        }

        /**
         * @brief Initialize server
         * 
         * @par Returns
         *  Nothing
        */
        static void init();

        /**
         * @brief Handles received (over some transport) bytes
         * 
         * @param [in] received Received frame
         * @param [out] response Response frame buffer
         * @param [out] response_size Bytes written into the response buffer
         * 
         * @retval true Answer (data transmit) required
         * @retval false Not answer (data transmit) required
        */
        static bool handle_rx(std::span<const uint8_t> received, std::span<uint8_t> response, unsigned& response_size);

    private:
        /**
         * @brief Handles the JSON envelope carried by one frame
         * 
         * @param [in] payload Received envelope
         * @param [out] response Response payload buffer
         * @param [out] response_size Bytes written into the response buffer
         * 
         * @retval true Answer (data transmit) required
         * @retval false Not answer (data transmit) required
        */
        static bool handle_payload(std::string_view payload, std::span<char> response, unsigned& response_size);
    };
}
#include "impl/rpc_server.h"

#endif //! RPC_RPC_SERVER_H_