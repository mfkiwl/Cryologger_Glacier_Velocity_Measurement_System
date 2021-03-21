/*
    Title:    Cryologger - Glacier Velocity Measurement System (GVMS) v2.0
    Date:     March 20, 2021
    Author:   Adam Garbo

    Components:
    - SparkFun Artemis Processor
    - SparkFun MicroMod Data Logging Carrier Board
    - SparkFun GPS-RTK-SMA Breakout - ZED-F9P (Qwiic)

    Comments:
    - Minimal debugging code for testing purposes
*/

// ----------------------------------------------------------------------------
// Libraries
// ----------------------------------------------------------------------------

#include <RTC.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h> // https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library
#include <SdFat.h>                                // https://github.com/greiman/SdFat
#include <SPI.h>
#include <WDT.h>
#include <Wire.h>

// -----------------------------------------------------------------------------
// Debugging
// -----------------------------------------------------------------------------
#define DEBUG       true   // Output debug messages to Serial Monitor
#define DEBUG_GNSS  false  // Output GNSS information to Serial Monitor

// ----------------------------------------------------------------------------
// Pin definitions
// ----------------------------------------------------------------------------

#define PIN_PWC_POWER     G1
#define PIN_QWIIC_POWER   G2
#define PIN_SD_CS         CS

// ----------------------------------------------------------------------------
// Object instantiations
// ----------------------------------------------------------------------------
APM3_RTC          rtc;
APM3_WDT          wdt;
SdFs              sd;
FsFile            file;
FsFile            debugFile;
SFE_UBLOX_GNSS    gnss;

// ----------------------------------------------------------------------------
// User defined global variable declarations
// ----------------------------------------------------------------------------
byte          sleepAlarmMinutes     = 5;  // Rolling minutes alarm
byte          sleepAlarmHours       = 0;  // Rolling hours alarm
byte          loggingAlarmMinutes   = 30; // Rolling minutes alarm
byte          loggingAlarmHours     = 0;  // Rolling minhoursutes alarm
byte          sleepAlarmMode        = 5;  // Alarm match on hundredths, seconds, minutes
byte          loggingAlarmMode      = 5;  // Alarm match on hundredths, seconds, minutes
unsigned int  gnssTimeout           = 5;  // Timeout for GNSS signal acquisition (minutes)

// ----------------------------------------------------------------------------
// Global variable declarations
// ----------------------------------------------------------------------------
const int     sdWriteSize         = 512;          // Write data to SD in blocks of 512 bytes
const int     fileBufferSize      = 16384;        // Allocate 16 KB RAM for UBX message storage
volatile bool alarmFlag           = false;        // Flag for alarm interrupt service routine
volatile bool loggingFlag         = false;        // Flag to determine if data logging should start/stop
volatile bool watchdogFlag        = false;        // Flag for watchdog timer interrupt service routine
volatile int  watchdogCounter     = 0;            // Counter for watchdog timer interrupts
bool          gnssConfigFlag      = false;        // Flag to track configuration of u-blox GNSS
bool          resetFlag           = false;        // Flag to force system reset using watchdog timer
bool          rtcSyncFlag         = false;        // Flag to indicate if the RTC was synced with the GNSS
char          fileName[30]        = "";           // Log file name
char          debugFileName[10]   = "debug.csv";  // Debugging log file name
unsigned int  maxBufferBytes      = 0;            // Maximum value of file buffer
unsigned long previousMillis      = 0;            // Global millis() timer
unsigned long bytesWritten        = 0;            // Counter for tracking bytes written to microSD

// ----------------------------------------------------------------------------
// Setup
// ----------------------------------------------------------------------------
void setup()
{
  // Pin assignments
  pinMode(PIN_QWIIC_POWER, OUTPUT);
  pinMode(PIN_PWC_POWER, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  qwiicPowerOn();       // Enable power to Qwiic connector
  peripheralPowerOn();  // Enable power to peripherials

  Wire.begin();         // Initalize I2C
  Wire.setPullups(0);   // Disable Artemis internal I2C pull-ups to reduce bus errors
  SPI.begin();          // Initialize SPI

  Serial.begin(115200); // Open Serial port
  //while (!Serial);      // Wait for user to open Serial Monitor
  delay(2000);          // Delay to allow user to open Serial Monitor

  Serial.println("Cryologger - Glacier Velocity Measurement System");

  printDateTime();      // Print RTC's current date and time

  // Configure devices
  configureWdt();       // Configure and start Watchdog Timer (WDT)
  configureGnss();      // Configure u-blox GNSS
  syncRtc();            // Acquire GNSS fix and sync RTC with GNSS
  configureSd();        // Configure microSD
  configureRtc();       // Configure real-time clock (RTC) alarm

  Serial.print("Info: Datetime is "); printDateTime();
  Serial.print("Info: Initial alarm set for "); printAlarm();

  // Blink LED to indicate completion of setup
  blinkLed(3, 1000);
  blinkLed(3, 100);
  blinkLed(3, 1000);
}

// ----------------------------------------------------------------------------
// Loop
// ----------------------------------------------------------------------------
void loop()
{
  // Check if alarm flag is set
  if (alarmFlag)
  {
    // Read RTC
    readRtc();

    // Clear alarm flag
    alarmFlag = false;

    Serial.print("Info: Alarm trigger "); printDateTime();

    // Toggle logging flag
    loggingFlag = !loggingFlag;

    // Perform measurements
    if (loggingFlag)
    {
      setLoggingAlarm();    // Set logging duration
      peripheralPowerOn();  // Enable power to peripherals
      configureSd();        // Configure microSD
      qwiicPowerOn();       // Enable power to Qwiic connector
      configureGnss();      // Configure u-blox GNSS
      logGnss();            // Log data
    }

    // Clear logging alarm flag
    alarmFlag = false;

    // Set the next RTC alarm
    setSleepAlarm();
  }

  // Check for watchdog interrupt
  if (watchdogFlag)
  {
    petDog(); // Restart watchdog timer
  }

  // Blink LED
  blinkLed(1, 25);

  // Enter deep sleep
  goToSleep();
}

// ----------------------------------------------------------------------------
// Interupt Service Routines (ISR)
// ----------------------------------------------------------------------------

// Interrupt handler for the RTC
extern "C" void am_rtc_isr(void)
{
  // Clear the RTC alarm interrupt
  //rtc.clearInterrupt();
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

  // Set alarm flag
  alarmFlag = true;
}

// Interrupt handler for the watchdog timer
extern "C" void am_watchdog_isr(void)
{
  // Clear the watchdog interrupt
  wdt.clear();

  // Perform system reset after 10 watchdog interrupts (should not occur)
  if (watchdogCounter < 10)
  {
    wdt.restart(); // Restart the watchdog timer
  }
  else
  {
    wdt.stop(); // Stop the watchdog timer
    peripheralPowerOff(); // Disable power to peripherals
    qwiicPowerOff(); // Disable power to Qwiic connector
    while (1)
    {
      blinkLed(2, 250);
      blinkLed(3, 1000);
    }
  }
  watchdogFlag = true; // Set the watchdog flag
  watchdogCounter++; // Increment watchdog interrupt counter
}
