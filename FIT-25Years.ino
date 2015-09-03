/*
 * MONASH UNIVERSITY
 * FACULTY OF INFORMATION TECHNOLOGY
 *
 * 25th Anniversary
 * Light Blue Bean Bluetoth LE sensor Arduino Code
 * Jon McCormack, August 2015
 *
 * Copyright (c) 2015 Monash University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses>.
 *
 */

#define NORMAL_SLEEP_TIME 5    // time in minutes to sleep between readings
#define PROBLEM_SLEEP_TIME 30    // time to sleep if there is a problem, e.g. low battery

// scratch banks -- used to store data that can be read by other BLE apps
#define PLANT_NAME_BANK 1
#define PLANT_SPECIES_BANK 2
#define DATA_BANK 3
#define TEXT_BANK 4
#define STATUS_BANK 5

// status bank organisation
// Byte   Value
//    0   Species type (0 if uninitialised)
//    1   minimum temperature
//    2   maximum temperature
//  3-4   minimum moisture
//  5-6   maximum moisture
//  7-8   current moisture
//  8-9   time since reset

// species names
#define BANKSIA             "Silver Banksia"
#define OLERIA              "Twiggy Daisy-bush"
#define KUNZEA              "Yarra Burgan"
#define BULBINE             "Leek Lilly"
#define CHRYSOCEPHALUM      "Yellow Buttons"
#define RHODANTHE           "Chamomile Sunray"
#define POA                 "Sword Tussock-grass"
#define AUSTROSTIPA         "Supple Spear-grass"

// Warning Levels
#define LOW_MOISTURE        40
#define LOW_TEMPERATURE      5
#define HIGH_TEMPERATURE    30
#define LOW_BATTERY         10

// Default Plant name
#define DEFAULT_PLANT_NAME  "My Plant"
#define DEFAULT_TEXT_MSG    "FIT - 25 years. Gift Bean V1.0"

#define BLEVEL 255
#define MINLEVEL 50

// Forward declare functions
uint16_t readSoilMoisture();


void setup()
{
  // initialize serial communication
  // Serial.begin(57600);
  pinMode(0, OUTPUT);     // set D0 as output
  digitalWrite(0, LOW);   // turn it off initially
  Bean.enableWakeOnConnect( true );

}

// the loop routine runs over and over again forever:
void loop()
{
  ScratchData status = getStatus(); // get this Bean's ststus
  if (status.length == 0 || status.data[0] == 0) {
    initStatus(DEFAULT_PLANT_NAME, BANKSIA, 1); // init this Bean
    Bean.sleep(100);
  }

  // get current min and max temperature and moisture readings
  uint8_t minTemp = status.data[1];
  uint8_t maxTemp = status.data[2];
  uint16_t minMoisture = status.data[3] | (status.data[4] << 8);
  uint16_t maxMoisture = status.data[5] | (status.data[6] << 8);
  uint16_t counter = status.data[9] | (status.data[10] << 8);

  uint8_t temp = Bean.getTemperature();
  uint8_t batteryLevel = Bean.getBatteryLevel();
  uint16_t moisture = readSoilMoisture();

  String beanName = getPlantName();
  beanName += " ";
  beanName += temp;
  beanName += "°C";
  if (moisture < LOW_MOISTURE)
    beanName += " TOO DRY";
  if (batteryLevel < LOW_BATTERY)
    beanName += " LOW BATT";

  Bean.setBeanName(beanName);
  if (batteryLevel < LOW_BATTERY)
    flash(255, 255, 0, 2);
  else if (temp > HIGH_TEMPERATURE)
    flash(255, 0, 0, 5);
  else if (moisture < LOW_MOISTURE)
    flash(0, 255, 0, 5);
  else if (temp < LOW_TEMPERATURE)
    flash(0, 0, 255, 5);
  else
    flash(255, 0, 255, 1); // all OK

  if (temp < minTemp)
    status.data[1] = temp;
  if (temp > maxTemp)
    status.data[2] = temp;
  if (moisture < minMoisture) {
    status.data[3] = lowByte(moisture);
    status.data[4] = highByte(moisture);
  }
  if (moisture > maxMoisture) {
    status.data[5] = lowByte(moisture);
    status.data[6] = highByte(moisture);
  }
  
  status.data[7] = lowByte(moisture);
  status.data[8] = highByte(moisture);
  
  ++counter;
  status.data[9] = lowByte(counter);
  status.data[10] = highByte(counter);

  String text = minTemp + "/" + maxTemp;
  text += "/";
  text += minMoisture;
  text += "/";
  text += maxMoisture;

  Bean.setScratchData(DATA_BANK, (const uint8_t *)text.c_str(), (text.length() + 1 > 20 ? 20 : text.length() + 1));
  Bean.setScratchData(TEXT_BANK, (const uint8_t *)beanName.c_str(), (beanName.length() + 1 > 20 ? 20 : beanName.length() + 1));
  Bean.setScratchData(STATUS_BANK, status.data, status.length);
  if (batteryLevel < LOW_BATTERY)
    Bean.sleep(PROBLEM_SLEEP_TIME * 60000);
  else
    Bean.sleep(NORMAL_SLEEP_TIME * 60000);
}

/*
 * setScratchBankText
 * sets the specified scratch bank to contain the supplied text
 */
void setScratchBankText(char text[], int bank) {
  if (strlen(text) > 20)
    text[19] = '\0';
  Bean.setScratchData(bank, (uint8_t *)text, strlen(text));
}

/*
 * readSoilMoisture
 * returns the current soil moisture reading from A1
 */
uint16_t readSoilMoisture() {
  digitalWrite(0, HIGH); // turn on voltage to one of the soil wires
  Bean.sleep(10);
  uint16_t sensorValue = analogRead(A1); // get reading (range 0 - 1024)
  digitalWrite(0, LOW); // turn off output to save power
  return sensorValue;
}

String getPlantName() {
  ScratchData name = Bean.readScratchData(PLANT_NAME_BANK);
  if (name.length == 0)
    return DEFAULT_PLANT_NAME;
  else {
    return String((const char *)name.data);
  }
}

/*
 * getStatus
 * return the status of this Bean from the STATUS_BANK scratch data
 *
 */
ScratchData getStatus() {
  ScratchData status = Bean.readScratchData(STATUS_BANK);
  return status;
}

/*
 * initStatus
 * one time initialisation of the scratch banks
 *
 */
void initStatus(char * myName, char * mySpecies, uint8_t mySpeciesNumber) {
  ScratchData plantName, plantSpecies, data, text, status;

  // Write the name of this Bean
  strcpy((char *)plantName.data, myName);
  plantName.length = strlen(myName) + 1;
  Bean.setScratchData(PLANT_NAME_BANK, plantName.data, plantName.length);

  // Write the species
  strcpy((char *)plantSpecies.data, mySpecies);
  plantSpecies.length = strlen(mySpecies) + 1;
  Bean.setScratchData(PLANT_SPECIES_BANK, plantSpecies.data, plantSpecies.length);

  // Write the text bank
  strcpy((char *)text.data, DEFAULT_TEXT_MSG);
  text.length = strlen(DEFAULT_TEXT_MSG) + 1;
  Bean.setScratchData(TEXT_BANK, text.data, text.length);

  // Write the data bank
  status.data[0] = mySpeciesNumber;
  status.data[1] = 0xFF;  // min temp
  status.data[2] = 0x00;  // max temp
  status.data[3] = 0xFF;  // min moisture
  status.data[4] = 0xFF;
  status.data[5] = 0x00;  // max moisture
  status.data[6] = 0x00;
  for (int i = 7; i < 11; ++i) // counter data
    status.data[i] = 0x00;
  status.length = 11;
  Bean.setScratchData(STATUS_BANK, status.data, status.length);

  // sleep
  Bean.sleep(100);

}


/*
 * flash
 * flash the bean's RGB LED in the specified colour and number of flashes
 *
 */
void flash(uint8_t r, uint8_t g, uint8_t b, int nTimes) {
  for (int i = 0; i < nTimes; ++i) {
    Bean.setLed(r, g, b);
    Bean.sleep(80);
    Bean.setLed(0, 0, 0);
    Bean.sleep(300);
  }
}
