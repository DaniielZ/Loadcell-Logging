#include "HX711.h"
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <Wire.h> //Lai izmantotu NEO-6M GPS
//#include <iomanip>      // std::setw funkcijai lai iestatītu kolonnas platumu
//pievieno sd liberARY
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 7;

HX711 scale;
File file;
bool recording = false;
bool create_new_file = false;
String path = "Data";
int path_number = 0;
int save_file_every = 80; // samples
int output_serial_every = 8; //outputs every sixteenth value (10 values per second)

void setup()
{
    delay(2000);
    Serial.begin(57600);
    Serial.println("Initializing the scale");
    pinMode(LOADCELL_DOUT_PIN, INPUT); // data line  //Yellow cable
    pinMode(LOADCELL_SCK_PIN, OUTPUT); // SCK line  //Orange cable
    pinMode(10, OUTPUT);               // for sd card
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    scale.set_scale(44350.0f / 1); // [kg] this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();                      // reset the scale to 0

    Serial.print("Initializing SD card...");// pārbauda vai ir SD karte
    if (!SD.begin(10))
    {
        Serial.println("SD card initialization failed!");
        while (1)
            ;
    }

    
    float result = 0;  //load cell lasījumam
    float result_summ = 0; // prieks videjas vertibas aprekinasanas katrā ciklā
    float result_max = -10000000;  // prieks lielakas vertibas noskaidrosanas katrā ciklā
    float result_min = 10000000;  // prieks mazakas vertibas noskaidrosanas katrā ciklā
    float all_time_max_force = -10000000; // visu laiku lielākais spēks
    unsigned long time = 0;
    unsigned long time_reset = 0; //priekš laika reset funkcijas
    int i = 0;

    while (true)
    {
        
        if (Serial.available()) 
        {
        switch (Serial.read())
            {
            case '1': all_time_max_force =0; Serial.println(F("All time max force is reset to 0")); break;  //resets all time max force
            case '0': all_time_max_force =0; time_reset = millis(); Serial.println(F("All time max force is reset to 0; Time is reset to 0")); break;  //resets all time max force and serial_time
            case 'r': recording = true; create_new_file = true; time_reset = millis(); Serial.println(F("Recording in proggres, press s to stop recording"));break; //starts recording
            case 's': recording = false; Serial.println(F("Recording stoped, press r to start recording"));break; //stops recording
            }
        }
    if(create_new_file){
    while (SD.exists(String(path + path_number + ".txt")))
    {
        path_number++;
    }
    file = SD.open(String(path) + String(path_number) + String(".txt"), FILE_WRITE);
    Serial.println(String(path) + String(path_number) + String(".txt"));
    Serial.println("initialization done.");
    file.println("Iteration, Force N, Time ms");
    file.close();
    create_new_file = false;
    }
        if(recording){
        if (i % save_file_every == 0)
        {
            file = SD.open(String(path) + String(path_number) + String(".txt"), FILE_WRITE);
            Serial.println("Open");
        }}
        if (i % save_file_every == 0)
        {
            Serial.println("Time milisec, min force[N], max force [N], average force [N], All time max [N]: " + String(all_time_max_force));
        }
        
        result = scale.get_units();
        result_summ = result_summ + result;
         if (all_time_max_force < result)
        {
            all_time_max_force = result;
        }
        if (result_max < result)
        {
            result_max = result;
        }
        if (result_min > result)
        {
            result_min = result;
        }

        time = millis();

                if (i % output_serial_every == 0)
        {
        Serial.print(i);
        Serial.print(",      ");
    //    Serial.print(result, 3);
    //    Serial.print(",      ");
        if(recording)
        {Serial.print(String("Recording ") + String(path) + String(path_number) + String(".txt")); break;}
        Serial.print(time - time_reset);
        Serial.print(",      ");
        Serial.print(result_min, 2);
        Serial.print(",      ");
        Serial.print(result_max, 2);
        Serial.print(",      ");
        Serial.print(result_summ/output_serial_every, 3);
        Serial.println();

        result_summ = 0;
        result_max = -10000000;
        result_min = 10000000;
        }

        if(recording){
        file.print(i);
        file.print(", ");
        file.print(result, 3);
        file.print(", ");
        file.print(time-time_reset);
        file.println();}

        i++;
        if(recording){
        if (i % save_file_every == 0)
        {
            Serial.println("Save");
            file.close();
        }}
    }
}

void loop()
{
}
