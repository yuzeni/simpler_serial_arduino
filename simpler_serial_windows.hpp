#pragma once

#include <iostream>
#include <Windows.h>
#include <cstdint>
#include <chrono>
#include <string>
#include <thread>

#ifdef NO_LOG_COLOR
# define BRIGHT_YELLOW ""
# define BRIGHT_RED ""
# define END_COLOR ""
#else
# define BRIGHT_YELLOW "\033[93m"
# define BRIGHT_RED    "\033[91m"
# define END_COLOR     "\033[0m"
#endif

#define COM_PORT_BASE "\\\\.\\COM"

namespace Simpler_Serial {

    /* Helper struct */

    template<typename T>
    union Byte_View
    {
        T val = T{};
        uint8_t bytes[sizeof(T)];
    };

    struct No_Delimiter {};

    /* Serial Handle
     * - establish the serial connection
     * - read serial packages as data, with delimiter or without delimiter
     */

    class Serial_Handle_Windows
    {
    public:

        Serial_Handle_Windows() {};
        
        // com_port : 0 => connect to the next available com port, otherwise connect to the specific port.
        Serial_Handle_Windows(uint16_t com_port, DWORD com_baud_rate, bool wait_for_connection = false)
        {
            this->init(com_port, com_baud_rate, wait_for_connection);
        }
        ~Serial_Handle_Windows();

        // com_port : 0 => connect to the next available com port, otherwise connect to the specific port.
        void init(uint16_t com_port, DWORD com_baud_rate, bool wait_for_connection);

        // delimiter        : Pick a byte sequence you would not expect to see in the package.
        // bytes_skip_limit : 0 => never skips bytes, otherwise skips bytes as long as the queue is longer than bytes_skip_limit
        template<typename VALUE_T, uint32_t PKG_SIZE, typename DELIM_T = No_Delimiter, DELIM_T delimiter = DELIM_T{}>
        bool read_serial_package(VALUE_T* package, uint32_t bytes_skip_limit = 0);
    
        bool write(uint8_t* bytes, size_t size);
    
        bool close_serial_port();
        bool is_connected() const { return this->m_connected; }

    private:
    
        HANDLE m_io_handler;
        COMSTAT m_status;
        DWORD m_errors;
        bool m_connected = false;
    };

    /* Serial Package
     * - create a package ready to be sent
     * - send the package through a Serial Handle
     */

    template <typename VALUE_T, uint32_t PKG_SIZE, typename DELIM_T, DELIM_T delim_>
    struct Delimited_Package
    {
        Delimited_Package () : delim(delim_)
        {
            static_assert(sizeof(*this) == sizeof(delim) + sizeof(values),
                          "The package should not be padded, choose different VALUE_T or DELIM_T.");
        }
        
        DELIM_T delim;
        VALUE_T values[PKG_SIZE];
    };

    template <typename T, uint32_t PKG_SIZE>
    struct Package
    {
        T values[PKG_SIZE];
    };

    template <typename PKG_CONTENT>
    struct Serial_Package
    {
        Serial_Package() {}
        
        bool send_package(Serial_Handle_Windows& serial)
        {
            return serial.write(as_bytes(), n_bytes);
        }
        
        auto& operator[](uint32_t idx)
        {
            return pkg.values[idx];
        }
        
        uint8_t* as_bytes()
        {
            return &bytes[0];
        }

        const size_t n_bytes = sizeof(PKG_CONTENT);

    private:

        union {
            PKG_CONTENT pkg{};
            uint8_t bytes[sizeof(PKG_CONTENT)];
        };
    };

    /* Serial Handle implementation */

    inline Serial_Handle_Windows::~Serial_Handle_Windows()
    {
        if (m_connected) {
            m_connected = false;
            CloseHandle(m_io_handler);
        }
    }

    inline void Serial_Handle_Windows::init(uint16_t com_port, DWORD com_baud_rate, bool wait_for_connection)
    {
        if (m_connected == true) {
            std::cout << BRIGHT_YELLOW "WARNING:" END_COLOR " Could not initialize COM port already connected.\n";
            return;
        }

        bool search_com_port = com_port == 0;
        int max_com_port_idx = 256;

        if (search_com_port)
        {
            com_port = 1;
            if (wait_for_connection) {
              std::cout << BRIGHT_RED "ERROR:" END_COLOR " Handle was not attached: Can't wait"
                  " for a specific connection and search through the COM ports at the same time.\n";
              return;
            }
        }

        std::string com_port_str = COM_PORT_BASE + std::to_string(com_port);
        
    retry_connection:
        
        m_io_handler = CreateFileA(com_port_str.c_str(),
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL,
                                   nullptr);

        if (m_io_handler == INVALID_HANDLE_VALUE)
        {
            if (search_com_port)
            {
                if (com_port > 256) {
                    std::cout << BRIGHT_RED "ERROR:" END_COLOR " Handle was not attached: Searched through "
                              << max_com_port_idx
                              << " COM ports and didn't find an open port.\n";
                    return;
                }
                
                com_port_str = COM_PORT_BASE + std::to_string(++com_port);
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
                goto retry_connection;
            }

            if (wait_for_connection)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                goto retry_connection;
            }
            
            DWORD error_code = GetLastError();
            
            switch(error_code)
            {
            case ERROR_FILE_NOT_FOUND:
                std::cout << BRIGHT_RED "ERROR:" END_COLOR " Handle was not attached: " << com_port << " not available\n";
                break;
                
            case ERROR_ACCESS_DENIED:
                std::cout << BRIGHT_RED "ERROR:" END_COLOR " Handle was not attached: Access to " << com_port
                          << " was denied. Likely due to an already established connection with another application.\n";
                break;
                
            default:
                std::cout << BRIGHT_RED "ERROR:" END_COLOR " Handle was not attached: An unexpected error occured. Error-Code: " << error_code << "\n";
                break;
            }
        }
        else
        {
            if (search_com_port) {
                std::cout << "INFO: Found and Connected to port COM" << com_port << "\n";
            }
            
            DCB dcbSerialParams = {};

            if (!GetCommState(m_io_handler, &dcbSerialParams)) {
                std::cout << BRIGHT_RED "ERROR:" END_COLOR " Failed to get current serial params\n";
            }
            else
            {
                dcbSerialParams.BaudRate = com_baud_rate;
                dcbSerialParams.ByteSize = 8;
                dcbSerialParams.StopBits = ONESTOPBIT;
                dcbSerialParams.Parity = NOPARITY;
                dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

                if (!SetCommState(m_io_handler, &dcbSerialParams)) {
                    std::cout << BRIGHT_RED "ERROR:" END_COLOR " could not set serial port params\n";
                }
                else {
                    m_connected = true;
                    PurgeComm(m_io_handler, PURGE_RXCLEAR | PURGE_TXCLEAR);
                }
            }
        }
    }

    template<typename VALUE_T, uint32_t PKG_SIZE, typename DELIM_T, DELIM_T delimiter>
    bool Serial_Handle_Windows::read_serial_package(VALUE_T* package, uint32_t bytes_skip_limit)
    {
        DWORD bytes_read;
        const size_t type_size = sizeof(VALUE_T);
        const size_t delim_size = std::is_same<DELIM_T, No_Delimiter>::value ? 0 : sizeof(DELIM_T);
        const size_t whole_package_size = type_size * PKG_SIZE + delim_size;
        const size_t package_size = type_size * PKG_SIZE;

        uint8_t whole_package[whole_package_size];
        Byte_View<DELIM_T> delim_test;
        int delim_search_idx = 0;

        bool read_pkg = false;

        ClearCommError(m_io_handler, &m_errors, &m_status);
        if (m_status.cbInQue >= whole_package_size)
        {
            if (ReadFile(m_io_handler, whole_package, whole_package_size, &bytes_read, nullptr))
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
                    ReadFile(m_io_handler, whole_package, delim_search_idx - delim_size, &bytes_read, nullptr);
                    std::cout << BRIGHT_YELLOW "WARNING:" END_COLOR " Skipped some bytes to realign to the data stream.\n";
                }
                else
                {
                    // If no delimiter was found at all, then step by just one byte.
                    ReadFile(m_io_handler, whole_package, 1, &bytes_read, nullptr);
                    std::cout << BRIGHT_YELLOW "WARNING:" END_COLOR " No delimiter was found in the current window. Shifting window by 1 byte.\n";
                }
            }
            else {
                std::cout << BRIGHT_YELLOW "WARNING:" END_COLOR " Failed to read " << PKG_SIZE << " byte(s).\n";
            }

            ClearCommError(m_io_handler, &m_errors, &m_status);
        }

        // optional queue skipping
        if (bytes_skip_limit && m_status.cbInQue > bytes_skip_limit)
        {
            uint8_t trash;
            int trash_cnt = 0;
            while (m_status.cbInQue > bytes_skip_limit)
            {
                ReadFile(m_io_handler, &trash, 1, &bytes_read, nullptr);
                ClearCommError(m_io_handler, &m_errors, &m_status);
                ++trash_cnt;
            }
            std::cout << BRIGHT_YELLOW "WARNING:" END_COLOR " skipped " << trash_cnt << " byte(s).\n";
        }
        
        return read_pkg;
    }

    inline bool Serial_Handle_Windows::write(uint8_t* bytes, size_t size)
    {
        DWORD bytes_sent;
        
        if (!WriteFile(m_io_handler, bytes, size, &bytes_sent, nullptr))
        {
            ClearCommError(m_io_handler, &m_errors, &m_status);
            return false;
        }
        else {
            return true;
        }
    }

    inline bool Serial_Handle_Windows::close_serial_port()
    {
        if (m_connected) {
            m_connected = false;
            CloseHandle(m_io_handler);
            return true;
        }
        else {
            return false;
        }
    }
}

#undef BRIGHT_YELLOW
#undef BRIGHT_RED
#undef END_COLOR

#undef COM_PORT_BASE
