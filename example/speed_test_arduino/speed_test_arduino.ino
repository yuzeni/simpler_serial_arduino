#include "../../simpler_serial_arduino.hpp"
#include "../package_definition.hpp"

using namespace Simpler_Serial;

void setup()
{
    Serial.begin(BAUD_RATE);
}

Serial_Package<Delimited_Package<VALUE_TYPE, PKG_SZ, DELIM_TYPE, DELIM_VALUE>> serial_package;

void loop()
{
    serial_package[0] = VALUE_TYPE{};
    serial_package.send_package();
}