// Configure microSD
void configureSd()
{
  // Start loop timer
  unsigned long loopStartTime = millis();

  // Initialize microSD
  if (!sd.begin(SdSpiConfig(PIN_SD_CS, DEDICATED_SPI)))
  {
    Serial.println("Warning: microSD failed to initialize.");
    online.microSd = false;
#if DEBUG_OLED
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "microSD failed to initialize.");
    u8g2.sendBuffer();
#endif
    peripheralPowerOff();
    qwiicPowerOff();
    while (1); // Force watchdog reset
  }
  else
  {
    online.microSd = true;
#if DEBUG_OLED
    u8g2.clearBuffer();
    u8g2.drawStr(0, 10, "microSD initialized.");
    u8g2.sendBuffer();
    delay(1000);
#endif
  }
  // Stop the loop timer
  timer.microSd = millis() - loopStartTime;
}

// Update the file create timestamp
void updateFileCreate()
{
  // Get the RTC's current date and time
  rtc.getTime();

  // Update the file create timestamp
  if (!logFile.timestamp(T_CREATE, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    Serial.print("Warning: Could not update file create timestamp.");
  }
}

// Update the file access and write timestamps
void updateFileAccess()
{
  // Get the RTC's current date and time
  rtc.getTime();

  // Update the file access and write timestamps
  if (!logFile.timestamp(T_ACCESS, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    Serial.print("Warning: Could not update file access timestamp.");
  }
  if (!logFile.timestamp(T_WRITE, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    Serial.print("Warning: Could not update file write timestamp.");
  }
}
