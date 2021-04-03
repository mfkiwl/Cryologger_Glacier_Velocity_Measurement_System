// Configure microSD
void configureSd()
{
  // Start loop timer
  unsigned long loopStartTime = millis();

  // Initialize microSD
  if (!sd.begin(SdSpiConfig(PIN_SD_CS, DEDICATED_SPI)))
  {
    DEBUG_PRINTLN("Warning: microSD failed to initialize. Reattempting...");
    online.microSd = false;

    blinkLed(2, 1000);
    peripheralPowerOff();
    qwiicPowerOff();
    // Force watchdog reset
    while (1)
    {
      blinkLed(1, 2000); // Force watchdog reset
      blinkLed(6, 250); // Force watchdog reset
    }
  }
  else
  {
    online.microSd = true;
  }
  // Stop the loop timer
  timer.microSd = millis() - loopStartTime;
}

// Update the file create timestamp
void updateFileCreate(FsFile *dataFile)
{
  // Get the RTC's current date and time
  rtc.getTime();

  // Update the file create timestamp
  if (!dataFile->timestamp(T_CREATE, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    DEBUG_PRINT("Warning: Could not update file create timestamp.");
  }
}

// Update the file access and write timestamps
void updateFileAccess(FsFile *dataFile)
{
  // Get the RTC's current date and time
  rtc.getTime();

  // Update the file access and write timestamps
  if (!dataFile->timestamp(T_ACCESS, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    DEBUG_PRINT("Warning: Could not update file access timestamp.");
  }
  if (!dataFile->timestamp(T_WRITE, (rtc.year + 2000), rtc.month, rtc.dayOfMonth, rtc.hour, rtc.minute, rtc.seconds))
  {
    DEBUG_PRINT("Warning: Could not update file write timestamp.");
  }
}
