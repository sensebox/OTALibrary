/*
 * Simple sketch that logs into an existing WiFi Network. After sending it listens for incoming
 * sketches on 192.168.0.46/sketch.
 */
#include <OTA.h>

OTA ota_module;
WiFiClient client;


char ssid[] = "YOURSSID"; //  your network SSID (name)
char pass[] = "YOURPASSWORD";             // your network password

const unsigned int postingInterval = 60e3;

bool validateFlashedApp() {
  /**
   * Test reset vector of application @APP_START_ADDRESS+4
   * Sanity check on the Reset_Handler address.
   * TODO FIXME: proper CRC / signature check?
   */

  /* Load the Reset Handler address of the application */
  uint32_t app_reset_ptr = *(uint32_t *)(APP_START_ADDRESS + 4);

  return (app_reset_ptr >= APP_START_ADDRESS && app_reset_ptr <= FLASH_SIZE);
}

void jumpToApp() {
  /* Load the Reset Handler address of the application */
  uint32_t app_reset_ptr = *(uint32_t *)(APP_START_ADDRESS + 4);

  LOG.print("app_reset_ptr: ");
  LOG.println(String(app_reset_ptr, HEX));

  LOG.print("stack pointer: ");
  LOG.println(String(*(uint32_t *)APP_START_ADDRESS, HEX));

  LOG.print("vector table address: ");
  LOG.println(String(((uint32_t)APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk), HEX));

  /* Rebase the Stack Pointer */
  __set_MSP(*(uint32_t *)APP_START_ADDRESS);

  /* Rebase the vector table base address */
  SCB->VTOR = ((uint32_t)APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

  /* Jump to application Reset Handler in the application */
  asm("bx %0" ::"r"(app_reset_ptr));
}


void setup() {
  pinMode(0, INPUT_PULLUP); // switch button

  #ifdef OTA_DEBUG
  LOG.begin(115200);
  while(!LOG) {;} // dont continue until serial was opened
  #endif

  // digitalRead(0) gives the value for the switch button (0 means its pressed)
  if (digitalRead(0) == LOW)
  {
    LOG.println("[OTA] switch pressed, entering OTA mode");
  } 
  else if (validateFlashedApp()) 
  {
    LOG.println("[OTA] jumping to application");
    jumpToApp();
  }
  else 
  {
    LOG.println("[OTA] no valid app installed, entering OTA mode");
  }
  // WINC1500 (WiFi-Bee) in XBEE1 Socket
  senseBoxIO.powerXB1(false); // power off to reset WINC1500
  delay(250);
  senseBoxIO.powerXB1(true); // power on
  

  // init WINC1500
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Serial.println("Error - Not Found");
    senseBoxIO.statusRed(); // status red
    // shutdown WINC1500
    WiFi.end();
    senseBoxIO.powerXB1(false);
    return; // don't continue
  }
  Serial.println("OK - Detected");

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the data:
  Serial.print("You're connected to the network; Setting up OTA functionalites");
  delay(400);
  ota_module.begin(false);

}

void loop() {
  unsigned long start = millis();
  // Do something every 60 seconds 
  Serial.println("Hello from Loop!");
  // Listen for sketches in between 
  for (;;)
  {
    // Start ota mode here; forget everything else
    unsigned long now = millis();
    unsigned long elapsed = now - start;
    ota_module.update();

    if (elapsed >= postingInterval)
      return;
  }
}
