#include "HX711.h"
#include <SPI.h>
#include <SD.h>
#include <Arduino.h>
#include <Wire.h> //Lai izmantotu NEO-6M GPS
// #include <iomanip>      // std::setw funkcijai lai iestatītu kolonnas platumu
// pievieno sd liberARY
//  HX711 circuit wiring
const int LOADCELL_DOUT_PIN = 8;
const int LOADCELL_SCK_PIN = 7;
// gps
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// The TinyGPS++ object
TinyGPSPlus gps;

static const int RXPin = 0, TXPin = 1;
static const uint32_t GPSBaud = 9600;

SoftwareSerial ss(RXPin, TXPin);

HX711 scale;
File file;
bool recording = false;
bool create_new_file = false;
String path = "Data";
int path_number = 0;
int save_file_every = 200;     // samples
int output_serial_every = 50;  // outputs every sixteenth value (10 values per second)
float all_time_max_force = -1; // visu laiku lielākais spēks
float last_speed = 0;          // pēdejais gp ātrums

unsigned long time_reset = 0; // priekš laika reset funkcijas
int measurement = 0;

struct DATA
{
    unsigned long time = 0;
    float force = 0;
    float speed = 0;
};

struct DATA_CYCLE
{
    float max_force = 0;
    float min_force = 0;
    float all_time_max_fore = 0;
    float average_force = 0;
    unsigned long last_time = 0;
    float speed_sum = 0;
    float average_speed = 0;
    float min_speed = 0;
    float max_speed = 0;
    float force_sum = 0;
    int measurements = 0;
};
void CreateNewFile()
{
    while (SD.exists(String(path + path_number + ".txt")))
    {
        path_number++;
    }
    file = SD.open(String(path) + String(path_number) + String(".txt"), FILE_WRITE);
    Serial.println(String(path) + String(path_number) + String(".txt"));
    Serial.println("initialization done.");
    file.println("Iteration, Force N, Time ms");
    file.close();
    time_reset = millis();
}
void CheckSerialInput()
{
    if (Serial.available())
    {
        switch (Serial.read())
        {
        case '1':
            all_time_max_force = 0;
            Serial.println(F("All time max force is reset to 0"));
            break; // resets all time max force
        case 'r':
            recording = true;
            CreateNewFile();
            Serial.println(F("Recording in proggres, press s to stop recording"));
            break; // starts recording
        case 's':
            recording = false;
            Serial.println(F("Recording stoped, press r to start recording"));
            break; // stops recording
        }
    }
}
float getGPSSpeed()
{
    float speed = 0;
    while (ss.available() > 0)
    {
        gps.encode(ss.read());
        if (gps.location.isUpdated())
        {
            // Number of satellites in use (u32)
            Serial.print("Number os satellites in use = ");
            Serial.println(gps.satellites.value());
            // Latitude in degrees (double)
            Serial.print("Latitude= ");
            Serial.print(gps.location.lat(), 6);
            // Longitude in degrees (double)
            Serial.print(" Longitude= ");
            Serial.println(gps.location.lng(), 6);
            Serial.print("Speed in m/s = ");
            Serial.println(gps.speed.mps());

            speed = gps.speed.mps();
        }
        if (speed == 0)
        {
            speed = last_speed;
        }
        last_speed = speed;
    }
    return speed;
}
DATA LogData(File file)
{
    DATA data;
    data.force = scale.get_units();
    data.time = millis() - time_reset;
    measurement++;
    data.speed = getGPSSpeed();

    file.print(measurement);
    file.print(", ");
    file.print(data.force, 3);
    file.print(", ");
    file.print(data.time);
    file.print(", ");
    file.print(data.speed, 3);
    file.println();
    return data;
}

void ProcessData(DATA_CYCLE &data_cycle, DATA data)
{
    // check for max
    if (data_cycle.max_force < data.force)
    {
        data_cycle.max_force = data.force;
    }
    // check for all time max
    if (all_time_max_force < data.force)
    {
        all_time_max_force = data.force;
    }

    // check for min
    if (data_cycle.min_force > data.force)
    {
        data_cycle.min_force = data.force;
    }
    // average
    data_cycle.measurements++;
    data_cycle.force_sum += data.force;
    data_cycle.average_force = data_cycle.force_sum / data_cycle.measurements;

    // set time
    data_cycle.last_time = data.time;

    // check for max
    if (data_cycle.max_speed < data.speed)
    {
        data_cycle.max_speed = data.speed;
    }

    // check for min
    if (data_cycle.min_speed > data.speed)
    {
        data_cycle.min_speed = data.speed;
    }
    // average
    data_cycle.speed_sum += data.speed;
    data_cycle.average_speed = data_cycle.speed_sum / data_cycle.measurements;
}

void PrintData(DATA_CYCLE &data_cycle)
{
    Serial.print("AvrgForce, N: ");
    Serial.print(data_cycle.average_force, 2);
    Serial.print("    ");
    Serial.print("MinForce, N: ");
    Serial.print(data_cycle.min_force, 2);
    Serial.print("    ");
    Serial.print("MaxForce, N: ");
    Serial.print(data_cycle.max_force, 2);
    Serial.print("    ");
    Serial.print("AllTimeForce, N: ");
    Serial.print(all_time_max_force, 2);
    Serial.print("    ");
    Serial.print("AvrgSpeed, km/h: ");
    Serial.print(data_cycle.average_speed * 3.6, 2);
    Serial.print("    ");
    Serial.print("MinSpeed, km/h: ");
    Serial.print(data_cycle.min_speed * 3.6, 2);
    Serial.print("    ");
    Serial.print("MaxSpeed, km/h: ");
    Serial.print(data_cycle.max_speed * 3.6, 2);
    Serial.print("    ");
    Serial.print("Time, ms: ");
    Serial.print(data_cycle.last_time);
    Serial.println();
}

void setup()
{
    delay(5000);
    Serial.begin(57600);
    Serial.println("Initializing the scale");
    pinMode(LOADCELL_DOUT_PIN, INPUT); // data line  //Yellow cable
    pinMode(LOADCELL_SCK_PIN, OUTPUT); // SCK line  //Orange cable
    pinMode(10, OUTPUT);               // for sd card
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

    scale.set_scale(44350.0f / 1); // [kg] this value is obtained by calibrating the scale with known weights; see the README for details
    scale.tare();                  // reset the scale to 0

    Serial.print("Initializing SD card..."); // pārbauda vai ir SD karte
    if (!SD.begin(10))
    {
        Serial.println("SD card initialization failed!");
        while (1)
        {
        }
    }
    Serial.println("initialized!");

    // gps
    ss.begin(GPSBaud);
    Serial.println("GPS initialized!");

    Serial.println("Press r to start recording.");
    while (true)
    {
        CheckSerialInput();
        if (recording)
        {
            file = SD.open(String(path) + String(path_number) + String(".txt"), FILE_WRITE);
            DATA_CYCLE data_cycle;
            for (int i = 0; i <= save_file_every; i++)
            {
                DATA data = LogData(file);
                ProcessData(data_cycle, data);
                if (i % output_serial_every == 0)
                {
                    PrintData(data_cycle);
                }
            }
            file.close();
            Serial.println("Save");
        }
    }
}
void loop()
{
}