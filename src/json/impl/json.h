/**
 * @file json.h
 * 
 * @brief Contains JSON definitions
*/

#ifndef JSON_IMPL_JSON_H_
#define JSON_IMPL_JSON_H_

#include "../../utils/converters.h"

#include <jsmn/jsmn.h>
#include <reflect/reflect>

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

namespace bomzhur {
    namespace private_impl {
        template < typename T >
        struct is_std_array : std::false_type
        {};

        template < typename T, size_t N >
        struct is_std_array< std::array< T, N > > : std::true_type
        {};

        template < typename T >
        constexpr bool is_std_array_v = is_std_array<T>::value;

        inline void parse_object(const char* data, const jsmntok_t* object, auto&& out);

        /**
         * @brief Parse one value into the given variable
         *
         * @param [in] data Data
         * @param [in] value Value token
         * @param [out] out Out value
        */
        inline void parse_value(const char* data, const jsmntok_t* value, auto&& out) {
            using type = std::decay_t<decltype(out)>;

            if constexpr (std::is_fundamental_v<type>) {
                convert_from_chars(&data[value->start], &data[value->end], out);
            } else if constexpr (is_std_array_v<type>) {
                const jsmntok_t* element = value + 1;
                for (unsigned i = 0; i < static_cast<unsigned>(value->size) && i < out.size(); ++i) {
                    parse_value(data, element, out[i]);
                    element = json_skip_value(element);
                }
            } else if constexpr (std::is_same_v<std::string_view, type>) {
                out = std::string_view(&data[value->start], &data[value->end]);
            } else if constexpr (std::is_standard_layout_v<type>) {
                parse_object(data, value, out);
            } else {
                static_assert(false, "Unsupported type");
            }
        }

        /**
         * @brief Parse object into the given struct
         *
         * @param [in] data Data
         * @param [in] object Object token (anything else has no members to walk)
         * @param [out] out Out value
        */
        inline void parse_object(const char* data, const jsmntok_t* object, auto&& out) {
            const jsmntok_t* key = object + 1;
            for (int i = 0; i < object->size; ++i) {
                const jsmntok_t* value = key + 1;
                auto parsed_key = std::string_view(&data[key->start], key->end - key->start);

                reflect::for_each([&](auto field) {
                    if (reflect::member_name<field>(out) == parsed_key) {
                        parse_value(data, value, reflect::get<field>(out));
                    }
                }, out);

                key = json_skip_value(value);
            }
        }
    }

    inline int json_tokenize(std::string_view json) {
        jsmn_parser parser;
        jsmn_init(&parser);

        int count = jsmn_parse(&parser, json.data(), json.size(), json_tokens, max_json_tokens);
        if (count < 0)
            return count;

        json_tokens[count].start = std::numeric_limits<int>::max();
        json_tokens[count].size = 0;

        for (const jsmntok_t* token = json_tokens; token < &json_tokens[count]; ++token) {
            if (token->type != JSMN_OBJECT)
                continue;

            const jsmntok_t* key = token + 1;
            for (int i = 0; i < token->size; ++i) {
                if (key->size != 1)
                    return JSMN_ERROR_INVAL;
                key = json_skip_value(key + 1);
            }
        }

        return count;
    }

    inline const jsmntok_t* json_skip_value(const jsmntok_t* value) {
        const jsmntok_t* next = value + 1;
        while (next->start < value->end)
            ++next;

        return next;
    }

    template<typename _type>
    consteval unsigned json_token_count() {
        using type = std::remove_cvref_t<_type>;

        if constexpr (std::is_fundamental_v<type> || std::is_same_v<type, std::string_view>) {
            return 1;
        } else if constexpr (private_impl::is_std_array_v<type>) {
            return 1 + std::tuple_size_v<type> * json_token_count<typename type::value_type>();
        } else {
            return []<std::size_t... _index>(std::index_sequence<_index...>) {
                return 1u + ((1u + json_token_count<decltype(reflect::get<_index>(std::declval<type&>()))>()) + ... + 0u);
            }(std::make_index_sequence<reflect::size<type>()>{});
        }
    }

    template<typename _type>
    consteval unsigned json_max_size() {
        using type = std::remove_cvref_t<_type>;

        if constexpr (std::is_same_v<type, bool>) {
            return 1;
        } else if constexpr (std::is_integral_v<type>) {
            return std::numeric_limits<type>::digits10 + 1 + (std::is_signed_v<type> ? 1 : 0);
        } else if constexpr (private_impl::is_std_array_v<type>) {
            return 1 + std::tuple_size_v<type> * (json_max_size<typename type::value_type>() + 1);
        } else if constexpr (std::is_same_v<type, std::string_view>) {
            static_assert(false, "A string has no upper bound on its JSON size");
        } else {
            return []<std::size_t... _index>(std::index_sequence<_index...>) {
                return 1u + ((reflect::member_name<_index, type>().size() + 3u
                    + json_max_size<decltype(reflect::get<_index>(std::declval<type&>()))>() + 1u) + ... + 0u);
            }(std::make_index_sequence<reflect::size<type>()>{});
        }
    }

    inline void update_json(std::string_view json, auto&& result) {
        // Methods without params get an empty span; malformed input lands here too
        if (json_tokenize(json) < 1)
            return;

        private_impl::parse_object(json.data(), &json_tokens[0], result);
    }

    template<typename _type>
    inline _type read_json(std::string_view json) {
        _type result{};
        update_json(json, result);
        return result;
    }

    inline char* write_json(char* where, auto&& data) {
        if constexpr (std::is_fundamental_v<std::decay_t<decltype(data)>>) {
            where = convert_to_chars(where, data);
        } else if constexpr (private_impl::is_std_array_v<std::decay_t<decltype(data)>>) {
            *where++ = '[';
            for (auto&& val : data) {
                where = write_json(where, val);
                *where++ = ',';
            }
            *(where - 1) = ']';
        } else if constexpr (std::is_same_v<std::string_view, std::decay_t<decltype(data)>>) {
            *where++ = '"';
            where = std::copy(data.begin(), data.end(), where);
            *where++ = '"';
        } else if constexpr (std::is_standard_layout_v<std::decay_t<decltype(data)>>) {
            *where++ = '{';

            reflect::for_each([&](auto field) {
                *where++ = '"';
                where = std::copy_n(reflect::member_name<field>(data).data(), reflect::member_name<field>(data).length(), where);
                *where++ = '"'; *where++ = ':';
                where = write_json(where, reflect::get<field>(data));
                *where++ = ',';
            }, data);
            *(where - 1) = '}';
            
        } else {
            static_assert(false, "Cannot dump type");
        }

        return where;
    }
}
#endif //! JSON_IMPL_JSON_H_
