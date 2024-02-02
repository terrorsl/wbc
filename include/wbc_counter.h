#ifndef WBC_COUNTER_FILE
#define WBC_COUNTER_FILE

enum CounterValueType
{
    LITER,
    CUBIC_METER
};
struct Counter
{
    Counter()
    {
        value=0;
        value_per_count=10;
    };
    unsigned char value_type;
    // value per count, example 1 count = 10 liter
    unsigned char value_per_count;
    // full value
    unsigned long value;
    unsigned long timestamp;

    char serial[32];
};

#endif