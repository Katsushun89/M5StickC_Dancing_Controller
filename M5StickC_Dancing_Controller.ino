
#include <M5StickC.h>
#include <esp_now.h>
#include <WiFi.h>

esp_now_peer_info_t slave;
#define CHANNEL 3
#define PRINTSCANRESULTS 0
#define DELETEBEFOREPAIR 0

void InitESPNow() {
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
}

void ScanForSlave() {
  int8_t scanResults = WiFi.scanNetworks();
  bool slaveFound = 0;  // reset on each scan
  memset(&slave, 0, sizeof(slave));
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);
     delay(10);
      if (SSID.indexOf("slave") == 0) {
        Serial.println("Found a Slave.");
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];
        if ( 6 == sscanf(BSSIDstr.c_str(), "%x:%x:%x:%x:%x:%x%c",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slave.peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        slave.channel = CHANNEL; // pick a channel
        slave.encrypt = 0; // no encryption
        slaveFound = 1;
        break;
      }
    }
  }
  if (slaveFound) {
    Serial.println("Slave Found, processing..");
  } else {
    Serial.println("Slave Not Found, trying again.");
  }
  WiFi.scanDelete();
}


bool manageSlave() {
  if (slave.channel == CHANNEL) {
    if (DELETEBEFOREPAIR) {
      deletePeer();
    }

    Serial.print("Slave Status: ");
    const esp_now_peer_info_t *peer = &slave;
    const uint8_t *peer_addr = slave.peer_addr;
    bool exists = esp_now_is_peer_exist(peer_addr);
    if ( exists) {
      // Slave already paired.
      Serial.println("Already Paired");
      return true;
    } else {
      // Slave not paired, attempt pair
      esp_err_t addStatus = esp_now_add_peer(peer);
      if (addStatus == ESP_OK) {
        // Pair success
        Serial.println("Pair success");
        return true;
      } else {
        Serial.println("Not sure what happened");
        return false;
      }
    }
  } else {
    Serial.println("No Slave found to process"); // No slave found to process
    return false;
  }
}

void deletePeer() {
  const esp_now_peer_info_t *peer = &slave;
  const uint8_t *peer_addr = slave.peer_addr;
  esp_err_t delStatus = esp_now_del_peer(peer_addr);
}

// send data
void sendData() {
  static int cnt = 0;
  uint8_t data[2];
  data[0] = cnt;
  data[1] = 100;
  const uint8_t *peer_addr = slave.peer_addr;
  Serial.print("Sending: "); //Serial.println(data);
  esp_err_t result = esp_now_send(peer_addr, data, 2);
  cnt++;
  Serial.print("Send Status: ");
  if (result == ESP_OK) {
    Serial.println("Success");
  } else {
    Serial.println("Not sure what happened");
  }
}

void setup()
{
  M5.begin();

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  InitESPNow();

  Serial.println("setup");
}

void loop()
{
  if (slave.channel == CHANNEL) { 
    bool isPaired = manageSlave();
    if (isPaired) {
      sendData();
    } else {
      Serial.println("Slave pair failed!");
    }
  }
  else {
    ScanForSlave();
  }
  delay(20);
}
