/**
 * @file rpc_server.h
 * 
 * @brief Contains rpc_server definitions
*/

#ifndef RPC_IMPL_RPC_SERVER_H_
#define RPC_IMPL_RPC_SERVER_H_

#include "../../json/json.h"
#include "../../utils/converters.h"
#include "../../utils/serialize.h"

#include <zhele/common/template_utils/type_list.h>
#include "../../utils/crc32.h"

#include <cstdint>
#include <limits>
#include <jsmn/jsmn.h>

#include <string.h>
#include <string_view>

namespace bomzhur::rpc {
    template<uint8_t _device_address, typename... _methods>
    inline void rpc_server<_device_address, _methods...>::init() {
    }

    template<uint8_t _device_address, typename... _methods>
    inline bool rpc_server<_device_address, _methods...>::handle_rx(std::span<const uint8_t> received, std::span<uint8_t> response, unsigned& response_size)
    {
        // The frame size arrives from the wire, so it has to be checked; the
        // response buffer is a compile-time array and needs no runtime guard
        if (received.size() < sizeof(rpc_frame_header))
            return false;

        auto rx_frame = reinterpret_cast<const rpc_frame_header*>(received.data());

        if (rx_frame->dst_address != _device_address)
            return false;

        uint16_t expected_frame_size = binary_deserialize<uint16_t>(rx_frame->frame_size);
        if (expected_frame_size != received.size() - sizeof(rpc_frame_header))
            return false;

        auto rx_payload = received.subspan(sizeof(rpc_frame_header));
        if (binary_deserialize<uint32_t>(rx_frame->crc32) != crc32::calculate(rx_payload))
            return false;

        auto tx_frame = reinterpret_cast<rpc_frame_header*>(response.data());
        auto tx_payload = response.subspan(sizeof(rpc_frame_header));

        unsigned tx_payload_size = 0;
        bool need_write = handle_payload(
            std::string_view(reinterpret_cast<const char*>(rx_payload.data()), rx_payload.size()),
            std::span(reinterpret_cast<char*>(tx_payload.data()), tx_payload.size()),
            tx_payload_size);

        if (!need_write)
            return false;

        response_size = tx_payload_size + sizeof(rpc_frame_header);
        tx_frame->dst_address = rx_frame->src_address;
        tx_frame->src_address = _device_address;
        binary_serialize(&tx_frame->frame_size[0], static_cast<uint16_t>(tx_payload_size));
        binary_serialize(&tx_frame->crc32[0], crc32::calculate(tx_payload.first(tx_payload_size)));

        return true;
    }

    template<uint8_t _device_address, typename... _methods>
    inline bool rpc_server<_device_address, _methods...>::handle_payload(std::string_view payload, std::span<char> response, unsigned& response_size) {
        const char* json = payload.data();

        // Shared buffer: method/params/id below are views over the receive buffer,
        // so the tokens are dead once they are taken and a handler may reuse them
        const auto* tokens = json_tokens;
        int token_count = json_tokenize(payload);

        std::string_view method;
        std::string_view id;
        std::string_view params;
        auto result = request_result_t::ok;

        if (token_count < 1 || tokens[0].type != JSMN_OBJECT) {
            return false;
        } else {
            // Walk the top-level members
            for (int key = 1; key + 1 < token_count; ) {
                const jsmntok_t& name = tokens[key];
                const jsmntok_t& value = tokens[key + 1];
                auto name_view = std::string_view(&json[name.start], name.end - name.start);

                if (name_view == _method_key) {
                    method = std::string_view(&json[value.start], value.end - value.start);
                } else if (name_view == _params_key) {
                    params = std::string_view(&json[value.start], value.end - value.start);
                } else if (name_view == _id_key) {
                    id = std::string_view(&json[value.start], value.end - value.start);
                }

                key = json_skip_value(&value) - tokens;
            }

            if (method.empty())
                result = request_result_t::invalid_request;
        }


        if (id.size() > _max_id_size)
            return false;

        // No id means a notification: act on it, but stay silent
        bool answer_required = !id.empty();

        // The handler writes behind the prefix, which is reserved up front
        auto result_area = response.subspan(_result_prefix.size(), response.size() - _result_prefix.size() - _answer_tail_size);
        unsigned result_size = 0;

        if (result == request_result_t::ok) {
            // A successful answer therefore needs no shifting afterwards
            result = request_result_t::method_not_found;

            methods_list::any([&](auto entry) {
                if (Zhele::template_utils::type_unbox<entry>::name == method) {
                    result = Zhele::template_utils::type_unbox<entry>::handler(params, result_area, result_size);
                    return true;
                }
                return false;
            });
        }

        if (!answer_required)
            return false;

        char* write = response.data();

        if (result == request_result_t::ok) {
            write = std::copy(_result_prefix.begin(), _result_prefix.end(), write);
            write = result_size > 0
                ? write + result_size
                : std::copy(_null_result.begin(), _null_result.end(), write);
        } else {
            auto message = result_message(result);

            write = std::copy(_error_prefix.begin(), _error_prefix.end(), write);
            *write++ = '-';
            write = convert_to_chars(write, static_cast<uint32_t>(-static_cast<int32_t>(result)));
            write = std::copy(_error_message_infix.begin(), _error_message_infix.end(), write);
            write = std::copy(message.begin(), message.end(), write);
            write = std::copy(_error_suffix.begin(), _error_suffix.end(), write);
        }

        write = std::copy(_id_infix.begin(), _id_infix.end(), write);
        write = std::copy(id.begin(), id.end(), write);
        *write++ = '}';

        response_size = write - response.data();

        return true;
    }
}
#endif //! RPC_IMPL_RPC_SERVER_H_
