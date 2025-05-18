#include "mbed.h"

// Shift register pins (Common Anode)
DigitalOut Latch(D4); // Latches data into storage registers
DigitalOut Clock(D7); // Shifts data on rising edge
DigitalOut Data(D8); // Serial data input

// Restarts the clock counter to 00:00 when pressed
DigitalIn S1(A1);
// Switch S3 displays ADC’s input voltage value in volts
DigitalIn S3(A3);
// Reads voltage from a potentiometer (0.0 to 3.3V)
AnalogIn potentiometer(A0);

// Common Anode four digit 7-segment display (0-9 with decimal point)
const uint8_t Digit_Pattern[10] = 
{
    0xC0, // 0
    0xF9, // 1
    0xA4, // 2
    0xB0, // 3
    0x99, // 4
    0x92, // 5
    0x82, // 6
    0xF8, // 7
    0x80, // 8
    0x90  // 9
};
// Digit positions from left to right of 7-segement
const uint8_t Digit_Position[4] = {0x01, 0x02, 0x04, 0x08};

// Timer and voltage variables
volatile int seconds = 0, minutes = 0; //00:00
volatile float Min_Volt = 3.3f, Max_Volt = 0.0f;
Ticker Timer; //Triggers a function at fixed time intervals(update the clock)

// Fucntion Prototype
void shiftOutMSBFirst(uint8_t value);
void writeToShiftRegister(uint8_t segment, uint8_t digit);
void updateTime();
void Display(int number, bool Show_Decimal = false, int Decimal_Position = -1);

// Shift register driver
void shiftOutMSBFirst(uint8_t value) 
{
    for(int i = 7; i >= 0; i--) // ensures the bits are sent in order from the highest bit (leftmost) to the lowest bit (rightmost)
    {
        Data = (value / (1 << i)) % 2; //creates a bitmask for the current bit position and shifts the desired bit to the LSB position and result is assigned to Data
        // Toggle Clock
        Clock = 1;
        Clock = 0;
    }
}

// Write data to shift register
void writeToShiftRegister(uint8_t segment, uint8_t digit) 
{
    Latch = 0;
    shiftOutMSBFirst(segment);
    shiftOutMSBFirst(digit);
    Latch = 1;
}

// Updating Time
void updateTime() 
{
    seconds = (seconds + 1) % 60;
    minutes = (minutes + (seconds == 0)) % 100; 
}

// Display number with optional decimal point
void Display(int number, bool Show_Decimal, int Decimal_Position) 
{
    int digit[4] = 
    {
        (number / 1000) % 10, // Thounsands place
        (number / 100) % 10, // Hundreds place
        (number / 10) % 10, // Tens place
        (number) % 10        // Unit place
    };

    for (int i = 0; i < 4; i++) 
    {
        uint8_t pattern = Digit_Pattern[digit[i]]; //Convert a digit from (0-9) into corresponding 7-segement encoding (0xC0-0x90)
        if (Show_Decimal && i == Decimal_Position) 
        {
        pattern &= ~0x80; // Turn OFF the MSB (common-anode: 0 = ON)
        }
        writeToShiftRegister(pattern, Digit_Position[i]);
        ThisThread::sleep_for(2ms);
    }
}

int main() {
    // Initialization
    S1.mode(PullUp);
    S3.mode(PullUp);
    Ticker Timer.attach(&updateTime, 1.0f);

    while (1) 
    {
        // Clock is reset to 00:00
        if (!S1) 
        {
            seconds = 0;
            minutes = 0;
            ThisThread::sleep_for(200ms); // Debounce
        }

        // Read voltage value from A0
        float Volt = potentiometer.read() * 3.3f;

        // Update Minimum and Maximum voltage
        if (Volt < Min_Volt) 
        {
        Min_Volt = Volt;
        }
        if (Volt > Max_Volt) 
        {
        Max_Volt = Volt;
        }

        // If S3 is released display time
        if (S3) 
        {
            int Time_Display = minutes * 100 + seconds;
            Display(Time_Display);
        } else //If S3 is pressed display voltage
        {
            int Voltage_Scaled = static_cast<int>(Volt * 100);
            Display(Voltage_Scaled, true, 1);
        }
    }
}