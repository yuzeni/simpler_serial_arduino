
struct Package_A
{
    int a = 1, b = 2, c = 3;
    float d = 43.8;
};

struct Package_B
{
    float arr[10] = {9,8,7,6,5,4,3,2,1,0};
    bool g = true;
};

struct Package_C
{
    Package_A a;
    Package_B b;
};

constexpr int BAUD_RATE = 256000; // Communication speed [bits/s (in the case of communication with arduino)]

constexpr int PKG_SZ = 100; // the number of values in a single package
                            //
// typedef int VALUE_TYPE; // change VALUE_TYPE to any type
typedef Package_A VALUE_TYPE;
// typedef Package_B VALUE_TYPE;
// typedef Package_C VALUE_TYPE;

// Use a delimiter type and value of your choice
typedef int DELIM_TYPE;
constexpr DELIM_TYPE DELIM_VALUE = -1;
