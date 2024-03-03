**ESP8266 - ESPeasy plugin for IKEA VINDSTYRKA** 

![](https://github.com/andibaciu/ESPeasy-plugin-for-IKEA-VINDSTYRKA/blob/main/img/sen5x_000.png "sen5x_000")

**Intro**

The next part of article is about sniffing i2c communication between IKEA controller and SEN5x sensor, this part is not researched by me, I take all this data from:

[https://github.com/just-oblivious/vindstyrka-docs]()

Ramblings about IKEA VINDSTYRKA and its sensor implementation.

**Sensor**

IKEA VINDSTYRKA contains a [Sensirion SEN54](https://sensirion.com/products/catalog/SEN54/) environmental sensor node, which can easily be interfaced with via I2C. The interesting bit is how IKEA implemented this sensor.

Implementations for ESPHome and Arduino are widely available, however these libraries yield different (and inaccurate) readings for temperature and humidity compared to what is displayed on IKEA VINDSTYRKA.

This discrepancy can be explained by the way IKEA processes the data, instead of using the preprocessed data from the sensor IKEA opted to use the raw sensor output (including undocumented mystery data! – about this mystery word I make some supposition) to compute the values for temperature, humidity and tVOC trend.

The goal of this exercise is to come up with a correction curve that can be used to accurately post-process the SEN54 humidity and temperature data.

**Sniffing**

VINDSTYRKA (V) sends the following commands to the SEN54 (S):

Once at startup:

V -> S: 0x00 0x21         Start measurements

Repeated roughly every second:

Dir.    Data              Description                       Note

V -> S: 0x02 0x01         CMD: Read data ready flag

V <- S: 0x00 0x01 CRC     Data ready flag response          [0]

V -> S: 0x03 0xC4         CMD: Read measured values

V <- S: MSB  LSB  CRC     PM1.0 reading \* 10 (uint16)

`        `MSB  LSB  CRC     PM2.5 reading \* 10 (uint16)       [1]

`        `MSB  LSB  CRC     PM4.0 reading \* 10 (uint16)

`        `MSB  LSB  CRC     PM10.0 reading \* 10 (uint16)

`        `MSB  LSB  CRC     Processed humidity \* 100 (uint16)

`        `MSB  LSB  CRC     Processed temperature \* 200 (uint16)

`        `MSB  LSB  CRC     Processed VOC Index \* 10 (uint16)

`        `MSB  LSB  CRC     Processed NOX Index \* 10 (uint16)

V -> S: 0x03 0xD2         CMD: Read raw measurements

V <- S: MSB  LSB  CRC     Raw humidity \* 100 (uint16)

`        `MSB  LSB  CRC     Raw temperature \* 200 (uint16)

`        `MSB  LSB  CRC     Raw VOC (uint16)                  [2]

`        `MSB  LSB  CRC     Raw NOX (uint16)

V -> S: 0x03 0xF5         CMD: Read raw mystery measurement

V <- S: MSB  LSB  CRC     Raw humidity \* 100 (uint16)       [3]

`        `MSB  LSB  CRC     Raw temperature \* 200 (uint16)    [4]

`        `???  ???  CRC     Mystery word                      [5]


**Footnotes:**

1. VINDSTYRKA respects the data ready flag and validates the CRC's.
1. VINDSTYRKA only appears to be using the PM2.5 reading from the processed values.
1. Raw VOC influences the tVOC trend arrow and VOC Index value
1. Value used to compute humidity.
1. Value used to compute temperature.
1. Undocumented bytes; influences humidity and temperature readings in a significant way.

**Addnotes:**

`	`After some research I was able to make some supposition about MYSTERY WORD and about how IKEA controller make some calculation and put on display some value, value not the same like you read from sen5x. This research is described in file: 

- sen5x\_logging\_data.xlsx 

(all sniffing data who stay on base of my calculus are from [https://github.com/just-oblivious/vindstyrka-docs]())

**Hardware**

I want to put inside IKEA Vindstyrka an ESP8266 modul D1mini and wirelink this device on i2c comunication to read data from SEN5x Sensirion sensor, BUT WITHOUT AFFECT I2C COMMUNICATION BETWEEN IKEA VINDSTYRKA CONTROLLER AND SEN5X SENSOR. For this implementation in our module D1mini we need another input pin to put this pin to monitor SCL pin from i2c communication and in this way to be able to start communication between D1mini and SEN5x AFTER communication between IKEA controller and sen5x finish.

![](https://github.com/andibaciu/ESPeasy-plugin-for-IKEA-VINDSTYRKA/blob/main/img/esp8266_d1mini_001.jpeg "esp8266_d1mini_001")

*D1mini ESP8266 module*

![](https://github.com/andibaciu/ESPeasy-plugin-for-IKEA-VINDSTYRKA/blob/main/img/sen5x_007.jpeg "sen5x_007")

*IKEA Vinstyrka pcb controller*

For hardware modification there are some wire link we must do between:

|<p>**D1 mini**</p><p>**pins**</p>|<p>**IKEA pcb controller**</p><p>**testpins**</p>|<p>**D1mini**</p><p>**pins**</p>|
| :-: | :-: | :-: |
|**5V**|VCC\_5V||
|**G**|GND2||
|**GPIO5(D1)**|SCL||
|**GPIO4(D2)**|SDA||
|**GPIO5(D1)**||GPIO13(D7)|

*Note: Wire link between D1 and D7 from ESP8266 module it’s for monitoring SCL pin from i2c communication.*

**ESPeasy esp8266 plugin**

There are 3 files for plugin:

- \_P167\_Vindtyrka.ino – put this file in folder: ESPeasy/scr
- P167\_data\_struct.h and P167\_data\_struct.cpp – put this files in folder ESPeasy/src/src/PluginStructs/
- Don’t forget to go to file ESPeasy/src/src/CustomBuild/define\_plugin\_sets.h and in the end of section PLUGIN\_SET\_STABLE add line #define USES\_P167   // IKEA Vindstyrka

After that you can make a compile and load a D1mini esp8266 module with this firmware.

When you want to Add a new device, in dropdown list you’ll find “Environment - Sensirion SEN5x (IKEA Vindstyrka)” devices.

It is very important to name this Task “IKEA\_Vindstyrka” especially if you want to define another one  Environment - Sensirion SEN5x (IKEA Vindstyrka) devices to get more then 4 parameters (the maximum parameters allowed by one task) (*like in picture below*)

![](https://github.com/andibaciu/ESPeasy-plugin-for-IKEA-VINDSTYRKA/blob/main/img/espeasy_001.jpg "espeasy_001")

and for the first task you MUST define the SCL monitoring pin for i2c communication – in my case I select GPIO13(D7)

**Conclusion**

With this plugin you could pair it with standalone SEN54 or SEN55 Sensirion air quality sensors (but you can only read parameters).

Feel free to contribute if you have anything interesting. Let's keep it all in one place 🙂

