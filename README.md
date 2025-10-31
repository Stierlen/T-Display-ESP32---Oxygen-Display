# T-Display-ESP32 - Oxygen-Display
T-Display ESP32 code to read analog value from Aithre Altus Meso pressure sensor. 
GUI was designed in SquareLine Studio 1.5.3

First setup with 3D printed connector extension and black display background. 

<img width="546" height="509" alt="image" src="https://github.com/user-attachments/assets/5498c1be-9e7f-4d6f-a708-7a7f88e6c0fe" />


## T-Display - Wiring
The TFT display should be easier to read in sunlight than the AMOLED display.

The analog inputs can only handle a maximum of 3.3V. Since the Meso sensor delivers up to 4.5V, a voltage divider must be installed. Otherwise, the analog inputs may be damaged. 

I simply used two identical resistors. Since the analog input of the ESP32 converts 12 bits (4095), there is no significant loss of resolution for this application.



<img width="799" height="523" alt="image" src="https://github.com/user-attachments/assets/2f48797a-696a-40a9-a2d1-993ee24752df" />
<br>

<img width="186" height="310" alt="image" src="https://github.com/user-attachments/assets/5d2f9d75-e90f-4bd3-81da-4af690e826f8" />

<img width="344" height="298" alt="image" src="https://github.com/user-attachments/assets/d6c47360-1114-4560-95dc-9c8b86c043c4" />
<br>
<img width="432" height="576" alt="image" src="https://github.com/user-attachments/assets/c80c7bdf-f6d1-4ffa-a55f-e7e1e3c7feca" />
<br>

The time forecast for the remaining oxygen quantity is based on a regression method, whereby the parameters have not yet been adjusted to the actual pressure drop magnitudes. If no pressure drop is registered for a longer period of time, the theoretical oxygen duration for a requirement at 6000 m is assumed, which is indicated by the label 6K.              

## Sensor plug connection
The Meso sensor is equipped with a 3-pin plug connection. The following female connector fits: “Cable repair kit, pressure switch (air conditioning) Loro 120-00-137”. The contacts must be crimped. 

Caution: the contacts can only be inserted from the front. Therefore, first pull the cable through the plug, crimp it with the contact, and only then press it into the plug.

<img width="376" height="323" alt="image" src="https://github.com/user-attachments/assets/36b219e7-6c7d-4bfb-b578-c2c28849acd5" />

<img width="307" height="384" alt="image" src="https://github.com/user-attachments/assets/0d669626-4339-4ca2-b167-728c9990d870" />

The connector can be kept significantly lower by drilling a hole directly to the side for the cable outlet. The cable should be reinforced in this area using heat-shrink tubing. Once the contacts have been crimped and pressed in, the open area on the back of the plug can be filled with hot glue. To smooth the surface of the pressed-in hot glue, it can be melted and smoothed using a small burner flame.

<img width="241" height="474" alt="image" src="https://github.com/user-attachments/assets/9bb6fcd8-5ceb-4865-8d55-38d18b27074e" />
<img width="503" height="406" alt="image" src="https://github.com/user-attachments/assets/a86d5f76-0afb-476b-baf9-9ed68236a0cf" />
<img width="485" height="420" alt="image" src="https://github.com/user-attachments/assets/2d5ced4f-a4be-4f43-92e5-96da7ae43710" />





## IDE Installation and Configuration
1. Install the Arduino IDE as described.
2. Add the folowing links in the IDE at line: "Additional boards managers URLs:".
   
    https://dl.espressif.com/dl/package_esp32_index.json
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json

4. Install the following libraries via the IDE library manager:
- Board library:  ESP32 (from Espressif) - Version 2.0.14
- Utilities - Version 0.4.6
- lvgl library - Version 8.3.0

Check the readme of the lvgl library. The file "lv_conf.h" must be copied to the libraries folder parallel to the lvgl folder.
Also check in "lv_conf.h" that all neccessary "LV_FONT_MONTSERRAT" - Fonts are enabled by flag 1.

- LV_FONT_MONTSERRAT_14 1
- LV_FONT_MONTSERRAT_16 1
- LV_FONT_MONTSERRAT_20 1
- LV_FONT_MONTSERRAT_30 1


5. Select the Board: "ESP32S3 Dev Module"
6. Select the following Board parameters
<img width="268" height="389" alt="image" src="https://github.com/user-attachments/assets/fa7b4c2a-d7c9-4cf8-8f83-85d30c82c155" />

7. Connect your T-Display to your PC
8. Choose the PORT connected to the T-Display

## Program adaptation 
1. Open "O2_display.ino"
2. Change the relevant parameters due to your requirements
  - sensorPin - Change to the number of the analog Input Pin you want to use on your display
  - druckFaktor - This parameter depends on your Meso Sensor output voltage and the resistors of your voltage divider
  - bottleLiters - Volume of your bottle
  - P_rest - Minimum allowed remaining pressure in the bottle
  - BAR_FULL_SCALE_BAR - Bar size corrrelation to max. pressure
  - analogWrite(38, 220); - brightness value (max. 255)

## Program Upload
1. Connect your Display to the selected Port
2. On the T-Display board press the UPLOAD BUTTON first and then the RESET BUTTON. Release the buttons again and the T-Display should now be in Uplode mode.
3. In the IDE -> Press the UPLOAD BUTTON. The code should now be transfered to the board.
4. The upload will take a few minutes. Wait until the it is finished.
5. Press the RESET BUTTON at the board again to reboot it.


