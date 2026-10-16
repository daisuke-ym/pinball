// ----------------------------------------------------------------------
void test_mp3() {
  if (digitalRead(SW1) == LOW) {
    delay(20);
    if (digitalRead(SW1) == LOW) {
      int num = random(1, 8);
      Serial.println("Playing MP3B track number: " + String(num));
      MP3B.play(num);
      if (MP3B.available()) {
        printDetail(MP3B.readType(), MP3B.read()); //Print the detail message from DFPlayer to handle different errors and states.
      }
      while (digitalRead(SW1) == LOW) {
        delay(20);
      }
    }
  }

  if (digitalRead(SW2) == LOW) {
    delay(20);
    if (digitalRead(SW2) == LOW) {
      int num = random(1, 13);
      Serial.println("Playing MP3J track number: " + String(num));
      MP3J.play(num);
      if (MP3J.available()) {
        printDetail(MP3J.readType(), MP3J.read()); //Print the detail message from DFPlayer to handle different errors and states.
      }
      while (digitalRead(SW2) == LOW) {
        delay(20);
      }
    }
  }

  if (digitalRead(SW3) == LOW) {
    delay(20);
    if (digitalRead(SW3) == LOW) {
      MP3B.pause();
      while (digitalRead(SW3) == LOW) {
        delay(20);
      }
    }
  }

  if (digitalRead(SW4) == LOW) {
    delay(20);
    if (digitalRead(SW4) == LOW) {
      MP3J.pause();
      while (digitalRead(SW4) == LOW) {
        delay(20);
      }
    }
  }

}

void printDetail(uint8_t type, int value){
  switch (type) {
    case TimeOut:
      Serial.println(F("Time Out!"));
      break;
    case WrongStack:
      Serial.println(F("Stack Wrong!"));
      break;
    case DFPlayerCardInserted:
      Serial.println(F("Card Inserted!"));
      break;
    case DFPlayerCardRemoved:
      Serial.println(F("Card Removed!"));
      break;
    case DFPlayerCardOnline:
      Serial.println(F("Card Online!"));
      break;
    case DFPlayerUSBInserted:
      Serial.println("USB Inserted!");
      break;
    case DFPlayerUSBRemoved:
      Serial.println("USB Removed!");
      break;
    case DFPlayerPlayFinished:
      Serial.print(F("Number:"));
      Serial.print(value);
      Serial.println(F(" Play Finished!"));
      break;
    case DFPlayerError:
      Serial.print(F("DFPlayerError:"));
      switch (value) {
        case Busy:
          Serial.println(F("Card not found"));
          break;
        case Sleeping:
          Serial.println(F("Sleeping"));
          break;
        case SerialWrongStack:
          Serial.println(F("Get Wrong Stack"));
          break;
        case CheckSumNotMatch:
          Serial.println(F("Check Sum Not Match"));
          break;
        case FileIndexOut:
          Serial.println(F("File Index Out of Bound"));
          break;
        case FileMismatch:
          Serial.println(F("Cannot Find File"));
          break;
        case Advertise:
          Serial.println(F("In Advertise"));
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}
