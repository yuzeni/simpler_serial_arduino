#include <chrono>

#include "../simpler_serial_windows.hpp"
#include "package_definition.hpp"

int main()
{
    using namespace Simpler_Serial;
    namespace chr = std::chrono;
    
    // 0 => search for an open com port, you can also specify the specific com port such as 3
    Serial_Handle_Windows serial(0, BAUD_RATE);
    
    VALUE_TYPE data[PKG_SZ];
    
    auto time_b = chr::time_point_cast<chr::microseconds>(chr::system_clock::now());
    VALUE_TYPE default_value = VALUE_TYPE{};
    
    while(true)
    {
        auto time_a = chr::time_point_cast<chr::microseconds>(chr::system_clock::now());
        
        if (serial.read_serial_package<VALUE_TYPE, PKG_SZ, DELIM_TYPE, DELIM_VALUE>(data))
        {
            // check if at least the first element is correct
            bool value_correctness = memcmp(&data[0], &default_value, sizeof(VALUE_TYPE)) == 0;
            
            double dt_ms = (time_a.time_since_epoch().count() - time_b.time_since_epoch().count()) / 1000.0;
            printf("data correctness: %s | throughput: %f KB/s\n",  value_correctness ? "true" : "false", double(PKG_SZ * sizeof(VALUE_TYPE)) / dt_ms);
            time_b = time_a;
        }
    }
}

