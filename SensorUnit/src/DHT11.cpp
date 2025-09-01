#include <DHT11.h>

DHT11::DHT11(int INPUT_PIN)
    : input_pin(INPUT_PIN) {}

byte DHT11::read_data()
{
    byte data;
    for (int i = 0; i < 8; i++)
    {
        if (digitalRead(input_pin) == LOW)
        {
            while (digitalRead(input_pin) == LOW) // Wait for the start of the data bit (line goes HIGH)
            {
            }
            delayMicroseconds(30); // Wait 30us, then sample the line to determine if bit is 0 or 1
            if (digitalRead(input_pin) == HIGH)
            {
                data |= (1 << (7 - i)); // Store '1' at correct bit position
            }
            while (digitalRead(input_pin) == HIGH) // Wait for the end of the data bit (line goes LOW)
            {
            }
        }
    }
    return data;
}

void DHT11::poll_sensor()
{
    this->lastPolled = millis();
    pinMode(input_pin, OUTPUT);
    digitalWrite(input_pin, LOW);  // Start signal: pull bus low
    delay(30);                     // Hold low for >18ms for DHT11 to detect
    digitalWrite(input_pin, HIGH); // Release bus
    delayMicroseconds(40);         // Wait for DHT11 response
    pinMode(input_pin, INPUT);
    while (digitalRead(input_pin) == HIGH) // Wait for DHT11 to pull bus low (response start)
    {
    }
    delayMicroseconds(80);             // DHT11 pulls bus low for 80us
    if (digitalRead(input_pin) == LOW) // Wait for DHT11 to pull bus high (response end)
    {
    }
    delayMicroseconds(80);      // DHT11 pulls bus high for 80us
    for (int i = 0; i < 4; i++) // Read 4 bytes of data (humidity and temperature)
    {
        dat[i] = read_data();
    }
    pinMode(input_pin, OUTPUT);
    digitalWrite(input_pin, HIGH); // Release bus for next communication
}

float DHT11::getTemperature()
{
    if (millis() >= (this->lastPolled + 1000))
    {
        poll_sensor();
    }
    return float(dat[2]) + float(dat[3]) / 10.0;
}

float DHT11::getHumidity()
{
    if (millis() >= (this->lastPolled + 1000))
    {
        poll_sensor();
    }
    return float(dat[0]) + float(dat[1]) / 10.0;
}

void DHT11::begin()
{
    poll_sensor();
}
