/**
 * @file json.h
 * 
 * @brief Contains simple (not powerful) json deserializer and serializer
*/

#ifndef JSON_JSON_H_
#define JSON_JSON_H_

#include <jsmn/jsmn.h>

#include <string_view>

namespace bomzhur {
    /**
     * @brief Token budget for one parse
    */
    inline constexpr unsigned max_json_tokens = 24;

    /**
     * @brief Shared jsmn token buffer
    */
    inline jsmntok_t json_tokens[max_json_tokens + 1];

    /**
     * @brief Tokenize JSON into json_tokens
     *
     * @param [in] json JSON text
     *
     * @returns Token count, or a negative jsmn error
    */
    inline int json_tokenize(std::string_view json);

    /**
     * @brief Skips a value together with everything nested in it
     *
     * @param [in] value Value token in json_tokens
     *
     * @returns First token after the value
    */
    inline const jsmntok_t* json_skip_value(const jsmntok_t* value);

    /**
     * @brief Parse JSON object into an existing value
     *
     * @param [in] json JSON text
     * @param [in, out] result Value to update
    */
    inline void update_json(std::string_view json, auto&& result);

    /**
     * @brief Parse JSON object into a new value
     *
     * @tparam _type Target type
     *
     * @param [in] json JSON text
     *
     * @returns Parsed value
    */
    template<typename _type>
    inline _type read_json(std::string_view json);

    /**
     * @brief Number of jsmn tokens a value of the type parses into
     *
     * @tparam _type Value type
     *
     * @returns Token count, known at compile time
    */
    template<typename _type>
    consteval unsigned json_token_count();

    /**
     * @brief Upper bound of write_json() output for the type
     *
     * @tparam _type Value type (strings are unbounded and rejected)
     *
     * @returns Size in bytes, known at compile time
    */
    template<typename _type>
    consteval unsigned json_max_size();

    /**
     * @brief Convert given data into JSON
     * 
     * @param [in] where Destination buffer
     * @param [in] data Data to convert
     * 
     * @returns Pointer to char after written JSON
    */
    inline char* write_json(char* where, auto&& data);
}

#include "impl/json.h" 

#endif //! JSON_JSON_H_