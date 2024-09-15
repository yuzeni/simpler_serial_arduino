#pragma once

namespace Simpler_Serial {

    /* Helper struct */
    
    template<typename T>
    union Byte_View
    {
        T val;
        byte bytes[sizeof(T)];
    };

    struct No_Delimiter {};

    /* read serial package
     * - read serial packages as data, with delimiter or without delimiter
     */

    // delimiter        : Pick a byte sequence you would not expect to see in the package.
    // bytes_skip_limit : 0 => never skips bytes, otherwise skips bytes as long as the queue is longer than bytes_skip_limit
    template<typename VALUE_T, unsigned int PKG_SIZE, typename DELIM_T = No_Delimiter, DELIM_T delimiter = DELIM_T{}>
    bool read_serial_package(VALUE_T* package, unsigned int bytes_skip_limit = 0)
    {
        const size_t type_size = sizeof(VALUE_T);
        const size_t delim_size = std::is_same<DELIM_T, No_Delimiter>::value ? 0 : sizeof(DELIM_T);
        const size_t whole_package_size = type_size * PKG_SIZE + delim_size;
        const size_t package_size = type_size * PKG_SIZE;

        byte whole_package[whole_package_size];
        Byte_View<DELIM_T> delim_test;
        int delim_search_idx = 0;

        bool read_pkg = false;

        if (Serial.available() >= whole_package_size)
        {
            if (Serial.readBytes(whole_package, whole_package_size))
            {
                if constexpr (!std::is_same<DELIM_T, No_Delimiter>::value)
                {
                    delim_search_idx = delim_size;
                
                    for (int i = 0; i < delim_size; ++i) {
                        delim_test.bytes[i] = whole_package[i];
                    }

                    while (delim_test.val != delimiter && delim_search_idx < whole_package_size)
                    {
                        for (int i = 0; i < delim_size - 1; ++i) {
                            delim_test.bytes[i] = delim_test.bytes[i + 1];
                        }
                    
                        delim_test.bytes[delim_size - 1] = whole_package[delim_search_idx];
                        ++delim_search_idx;
                    }
                }

                if (whole_package_size - delim_search_idx >= package_size)
                {
                    for(int p_i = 0; p_i < PKG_SIZE; ++p_i)
                    {
                        Byte_View<VALUE_T> type_value;
                        for (int s_i = 0; s_i < type_size; ++s_i) {
                            type_value.bytes[s_i] = whole_package[delim_search_idx + p_i * type_size + s_i];
                        }
                        package[p_i] = type_value.val;
                    }
                    read_pkg = true;
                }
                else if (delim_search_idx < whole_package_size)
                {
                    // Read the amount of bytes it takes to get back into alignment,
                    // such that the next read would, unlike this time, start at the delimiter.
                    Serial.readBytes(whole_package, delim_search_idx - delim_size);
                }
                else
                {
                    // If no delimiter was found at all, then step by just one byte.
                    Serial.readBytes(whole_package, 1);
                }
            }
        }
    
        return read_pkg;
    }

    /* Serial Package
     * - create a package ready to be sent
     * - send the package
     */

    template <typename VALUE_T, unsigned int PKG_SIZE, typename DELIM_T, DELIM_T delim_>
    struct Delimited_Package
    {
        Delimited_Package ()
        {
            static_assert(sizeof(*this) == sizeof(delim) + sizeof(values),
                          "The package should not be padded, choose different VALUE_T or DELIM_T.");
        }
        
        const DELIM_T delim = delim_;
        VALUE_T values[PKG_SIZE];
    };

    template <typename T, unsigned int PKG_SIZE>
    struct Package
    {
        T values[PKG_SIZE];
    };
    
    template <typename PKG_CONTENT>
    struct Serial_Package
    {
        Serial_Package() {}
        
        void send_package()
        {
            Serial.write(as_bytes(), n_bytes);
        }
        
        auto& operator[](unsigned int idx)
        {
            return pkg.values[idx];
        }
        
        byte* as_bytes()
        {
            return &bytes[0];
        }

        const size_t n_bytes = sizeof(PKG_CONTENT);

    private:

        union {
            PKG_CONTENT pkg{};
            byte bytes[sizeof(PKG_CONTENT)];
        };
    };
}
