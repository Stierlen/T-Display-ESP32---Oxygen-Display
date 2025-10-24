# T-Display-ESP32---Oxygen-Display
T-Display ESP32 code to read analog value from Aithre Altus Meso pressure sensor

# T-Display
The TFT display should be easier to read in sunlight than the AMOLED display. 

# IDE Installation and Configuration
1. Install the Arduino IDE as described.
2. Add the folowing links in the IDE at line: "Additional boards managers URLs:".
   
    https://dl.espressif.com/dl/package_esp32_index.json
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json

4. Install the following libraries via the IDE library manager:
- Board library:  ESP32 (from Espressif) - Version 2.0.14
- Utilities - Version 0.4.6
- lvgl library - Version 8.3.0

Check the readme of the lvgl library. The file "lv_conf.h" must be copied to the libraries folder parallel to the lvgl folder.

5. Select the Board: "ESP32S3 Dev Module"
6. Select the following Board parameters
<img width="268" height="389" alt="image" src="https://github.com/user-attachments/assets/fa7b4c2a-d7c9-4cf8-8f83-85d30c82c155" />

7. Connect your T-Display to your PC
8. Choose the PORT connected to the T-Display

# Program adaptation 
1. Open "O2_display.ino"
2. Change the relevant parameters due to your requirements
  - sensorPin - Change to the number of the analog Input Pin you want to use on your display
  - druckFaktor - This parameter depends on your Meso Sensor output voltage and the resistors of your voltage divider
  - bottleLiters - Volume of your bottle
  - P_rest - Minimum allowed remaining pressure in the bottle
  - BAR_FULL_SCALE_BAR - Bar size corrrelation to max. pressure
  - analogWrite(38, 220); - brightness value (max. 255)

# Program Upload
1. Connect your Display to the selected Port
2. On the T-Display board press the UPLOAD BUTTON first and then the RESET BUTTON. Release the buttons again and the T-Display should now be in Uplode mode.
3. In the IDE -> Press the UPLOAD BUTTON. The code should now be transfered to the board.
4. The upload will take a few minutes. Wait until the it is finished.
5. Press the RESET BUTTON at the board again to reboot it.


