# IoT-EASE
Project to build an accessible IoT input device.

This project was created by Jacob Rumpf, Takanori Ohashi, and John Kimura as a capstone project for the Computer Engineering program at Cal Poly. It was sponsored by alumnus Jackson Pang and supervised by Professor John Oliver.

The code must be compiled and run in the Arduino IDE for an ESP32 development board. It interacts with a Raspberry Pi running an MQTT server (must be set up separately) to send messages based on user inputs.

esp32_mqtt_setup.ino : Sets up a simple test for MQTT communication with the Raspberry Pi.
calibration_test.ino : Runs the calibration process for the muscle sensor and reads the sensor for testing.
version1.ino         : Combines MQTT and muscle sensor calibration to send a message when the sensor reads an input.

For a more thorough explanation of the project and hardware setup, consult the full documentation.
