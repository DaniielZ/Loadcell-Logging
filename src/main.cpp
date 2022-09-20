#include "HX711.h"
#include <SPI.h>
#include <SD.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 7;

HX711 scale;
File file;
String path = "Data";
int path_number = 0;
int save_file_every = 80; // samples

void setup()
{
    delay(2000);
    Serial.begin(57600);
    Serial.println("Initializing the scale");
    pinMode(LOADCELL_DOUT_PIN, INPUT); // data line  //Yellow cable
    pinMode(LOADCELL_SCK_PIN, OUTPUT); // SCK line  //Orange cable
    pinMode(10, OUTPUT);               // for sd card
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    scale.set_scale(-44350.0f / 9.81); // this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();                      // reset the scale to 0

    Serial.print("Initializing SD card...");
    if (!SD.begin(10))
    {
        Serial.println("SD card initialization failed!");
        while (1)
            ;
    }

    while (SD.exists(String(path + path_number + ".txt")))
    {
        path_number++;
    }
    file = SD.open(String(path) + String(path_number) + String(".txt"), FILE_WRITE);
    Serial.println(String(path) + String(path_number) + String(".txt"));
    Serial.println("initialization done.");
    file.println("Iteration, Time ms, Force N");
    file.close();