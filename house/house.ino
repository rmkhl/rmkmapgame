/*
 * pin layout:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino          Arduino
 *             Reader/PCD   Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10               10
 * SPI MOSI    MOSI         ICSP-4           16
 * SPI MISO    MISO         ICSP-1           14
 * SPI SCK     SCK          ICSP-3           15
 * 
 * LED                                       8
 */
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>

#define LED 8
byte runningMode = 0;        // current runing mode
byte ledStatus = LOW;        // current led status
unsigned long delayStarted;  // last time led status was changed

byte readCard[4];            // scanned ID read from RFID Module
byte teachingCard[4] = { 0x9C, 0x2F, 0x8C, 0xC3 };
byte locationCard[4];        // correct location card

byte teachingMatch = 0;
byte locationMatch = 0;

#define RST_PIN 9          // see pin layout above
#define SS_PIN  10         // see pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  pinMode(LED, OUTPUT);
	SPI.begin();
	mfrc522.PCD_Init();
	delay(4);
  digitalWrite(LED, ledStatus);
  delayStarted = millis();
  for (uint8_t i = 0; i < 4; i++) {
    locationCard[i] = EEPROM.read(i);
    Serial.print(locationCard[i], HEX);
  }
}

void loop() {
  // Check the mode we are in and handle the led blinking accordingly
  switch(runningMode) {
    case 0:  // seeking mode; led blinking slowly
      if ((millis() - delayStarted) >= 600) {
          delayStarted = millis();
          ledStatus = ledStatus^1;
          digitalWrite(LED, ledStatus);
      }
      break;
    case 1:  // found; led on
      ledStatus = HIGH;
      digitalWrite(LED, ledStatus);
      break;
    case 2:  // teaching mode; led blinking fast
      if ((millis() - delayStarted) >= 200) {
          delayStarted = millis();
          ledStatus = ledStatus^1;
          digitalWrite(LED, ledStatus);
      }
      break;
  }

  // See if we have seen a card
	if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return;
	}

	// Select one of the cards
	if ( ! mfrc522.PICC_ReadCardSerial()) {
		return;
	}

  // We have a new card, lets read the ID
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  }
  mfrc522.PICC_HaltA(); // Stop reading

  // See if it matches any of the known cards
  teachingMatch = 0;
  locationMatch = 0;
  for (uint8_t i = 0; i < 4; i++) {
    if (readCard[i] == teachingCard[i]) {
      teachingMatch++;
    }
    if (readCard[i] == locationCard[i]) {
      locationMatch++;
    }
  }

  // Teaching card?
  if (teachingMatch == 4) {
    runningMode = 2;  // Yup, go to teaching mode
    return;
  }
  
  // New card we are teaching?
  if (runningMode == 2) {
    // Save the card info
    for (uint8_t i = 0; i < 4; i++) {
      locationCard[i] = readCard[i];
      EEPROM.write(i, locationCard[i]);
    }
    runningMode = 1;  // and switch to found mode for success indication
    return;
  }

  // Correct location card?
  if (locationMatch == 4) {
    runningMode = 1;  // Yup, change to found mode
    return;
  }

  // None of the above, stay in seeking mode
  runningMode = 0;
}
