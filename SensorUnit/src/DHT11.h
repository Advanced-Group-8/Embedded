#ifndef DHT11_H
#define DHT11_H

#include <Arduino.h>

class DHT11
{
public:
    explicit DHT11(int INPUT_PIN);

    void begin();
    float getTemperature();
    float getHumidity();

private:
    int input_pin{};
    byte dat[5];
    unsigned long lastPolled{};

    void poll_sensor();
    byte read_data();
};

/* EXAMPLE USAGE:
  #include <Arduino.h>
  #include <DHT11.h>

  DHT11 dht11(2);

  void setup()
  {
      Serial.begin(115200);
  }

  void loop()
  {
      Serial.println(dht11.returnTemperature());
      delay(2500);
  }
*/

#endif