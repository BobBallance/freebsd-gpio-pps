# Wiring an AdaFruit Ultimate GPS Breakout to a PI-2 Model B

Here are the wiring connections that I used. Note that the Adafruit part level-shifts all outputs to 3.3v, so no additional resistors were needed. *This might not be true of your part!* **Use caution when hooking up any electronics to your PI.**

|PI | PI Pin || Adafruit Breakout Board|
|:=======|:===|:==|:===|
| 5v| 04 | —>| Vin |
| Gnd | 06 | —>| Gnd |
| GPIO04| 05 |<—| PPS |
| GPIO14 TX | 08 |—>| RX |
| GPIO15 RX | 10 |<—| TX |

The PI Pins and layouts are keyed to the diagram at [Element14](http://www.element14.com/community/docs/DOC-73950/l/raspberry-pi-2-model-b-gpio-40-pin-block-pinout).
