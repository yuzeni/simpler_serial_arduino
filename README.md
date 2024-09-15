
# Simpler serial communication between a Windows C++ application and an Arduino

This tiny library is based on [simple_serial_port](https://github.com/dmicha16/simple_serial_port), but more flexible, faster and perhaps even simpler to use.  
It was written for use with high speed ADCs.  

## Features

- **Universal**: Send/receive packages (arrays) of any C++ type over a serial port with a custom delimiter between packages.  
- **Fast**: Packages are sent and read at once, significantly increasing transfer speeds compared to [simple_serial_port](https://github.com/dmicha16/simple_serial_port), where single byte operations are used.
- **Simple to use**: See "How to" below.
- **Robust**: The delimiter ensures that the stream of data returns to alignment in the case of interruptions.

## How to use it

Include the `*_arduino.hpp` into your arduino project and the `*_windows.hpp` into your windows project.  

### On windows

**Initialization:**  
```cpp
using namespace Simpler_Serial;
Serial_Handle_Windows serial(3, 115200); // Your custom com port and baud rate.

// Set the com port to 0, to connect to the next open com port.
// Set 'wait_for_connection' to true (3, 115200, true), to wait on a specific com port.
```

**Sending packages:**  
```cpp
struct My_Data { int a, b, c; float d; };

// underlying package type = My_data
// package size = 5
// delimiter type = float
// delimiter value = INFINITY

Serial_Package<Delimited_Package<My_Data, 5, float, INFINTY>> serial_package_with_delimiter;
Serial_Package<Package<My_Data, 5>> serial_package_without_delimiter;

// fill the package with data
serial_package_with_delimiter[0] = {1, -2, 3, 4.5};
// ...

// send the package over the already initialized serial handle
serial_package_with_delimiter.send_package(serial);
```

**Receiving packages:**  
```cpp
My_Data data[5];

// underlying package type = My_Data
// package size = 5
// delimiter type = float
// delimiter value = INFINITY
// buffer = data

serial.read_serial_package<My_Data, 5, float, INFINITY>(data)>

// or this, if the package has no delimiter
serial.read_serial_package<My_Data, 5>(data)>
```

**Closing the serial port:**  
```cpp
seral.close_serial_port();
```

### On Arduino

**Initialization:**  
```cpp
using namespace Simpler_Serial;
// Make sure this matches the baud rate on the windows side (it can be any positive integer, as long they match)
Serial.begin(115200);
```

**Sending packages:**  
```cpp
// The only difference is that no serial handle has to be passed (Arduino already has 'Serial')
serial_package_with_delimiter.send_package();
```

**Receiving packages:**  
```cpp
// Again, the only difference is the missing explicit serial handle.
read_serial_package<My_Data, 5, float, INFINITY>(data)
```

### Important info


#### Delimiter trouble

Internally the demlimiter and an array of the package type are combined into a single struct, such that they can be sent/received at once.
Sometimes this leads to extra padding bytes between the two, which would break the communication.
In those situations a static_assert will trigger and ask you to change either the type of the delimiter or the package type.  

Alternatively you could also include the delimiter yourself, by making sure that the leading bytes of the package belong to the delimiter.  
In this case, you would create the package *without* delimiter, but act as if it has one, when receiving it.

#### Struct alignment trouble

If you are using custom data types, make sure that they are equivalent memory wise on both sides, especially watch padding as it is often not obvious.
Printing a `sizeof(My_Struct)` should uncover most discrepancies.

#### On the serial queue

- The Arduino Uno has a queue size of just 64 bytes. So any package (including the delimiter) has to fit in there, since they are read at once.
- If your Windows application struggles too keep up to the arduino (unlikely scenario), you can set the argument `bytes_skip_limit` of `read_serial_package`
to a value greater than 0 to skip bytes whenever the queue grows larger than this limit. This can also be used to synchronize faster with the incomming data stream,
when there is a lag spike (more likely scenario). On the arduino side this feature was emited, due to the small queue size.

#### On arduino warning messages

Since logging through `Serial.print()` would interfere with the serial packages,
no warnings are printed, when bytes are skipped for example.
