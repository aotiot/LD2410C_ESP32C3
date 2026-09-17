// ESP32-C3 mini + HLK-LD2410C ihmistunnistussensori
//
// Kytkentä (sensorin pinnit -> ESP32-C3):
//   Sensorin PIN1 (TX)  -> ESP32 RX_PIN  (ristiin, sensorin TX menee ESP32:n RX:ään)
//   Sensorin PIN2 (RX)  -> ESP32 TX_PIN  (ristiin, sensorin RX menee ESP32:n TX:ään)
//   Sensorin PIN3 (OUT, "human detected") -> ESP32 PRESENCE_PIN
//   Sensorin VCC -> 5V (tai 3.3V riippuen moduulista), GND -> GND
//
// HUOM: muuta alla olevat GPIO-numerot vastaamaan omaa kytkentääsi.
#define RX_PIN        1   // ESP32-C3 GPIO, kytketty sensorin TX-pinniin (pin1)
#define TX_PIN        2   // ESP32-C3 GPIO, kytketty sensorin RX-pinniin (pin2)
#define PRESENCE_PIN  3   // ESP32-C3 GPIO, kytketty sensorin OUT-pinniin (pin3)

#define LD2410_BAUD   256000  // LD2410C:n oletusnopeus

HardwareSerial LD2410Serial(1); // käytetään ESP32:n UART1:tä

// LD2410 datakehyksen kehysmerkit (normaali/lyhyt raportointimuoto)
static const uint8_t FRAME_HEADER[4] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t FRAME_FOOTER[4] = {0xF8, 0xF7, 0xF6, 0xF5};

uint8_t frameBuf[64];
size_t frameLen = 0;
bool inFrame = false;

struct LD2410Data {
  bool valid = false;
  uint8_t targetState = 0;       // 0=ei kohdetta, 1=liikkuva, 2=paikallaan, 3=molemmat
  uint16_t movingDistanceCm = 0;
  uint8_t movingEnergy = 0;
  uint16_t staticDistanceCm = 0;
  uint8_t staticEnergy = 0;
  uint16_t detectionDistanceCm = 0;
} ld2410;

void setup() {
  Serial.begin(115200);
  pinMode(PRESENCE_PIN, INPUT);

  LD2410Serial.begin(LD2410_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);

  Serial.println("HLK-LD2410C luku kaynnistetty");
}

void loop() {
  readLD2410();

  bool presence = digitalRead(PRESENCE_PIN) == HIGH;

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 500) {
    lastPrint = millis();

    Serial.print("Ihminen havaittu (OUT-pinni): ");
    Serial.print(presence ? "KYLLA" : "EI");

    if (ld2410.valid) {
      Serial.print(" | UART tila: ");
      switch (ld2410.targetState) {
        case 0: Serial.print("ei kohdetta"); break;
        case 1: Serial.print("liikkuva"); break;
        case 2: Serial.print("paikallaan"); break;
        case 3: Serial.print("liikkuva+paikallaan"); break;
        default: Serial.print("tuntematon"); break;
      }
      Serial.print(" | liikkuva: ");
      Serial.print(ld2410.movingDistanceCm);
      Serial.print("cm (energia ");
      Serial.print(ld2410.movingEnergy);
      Serial.print(") | paikallaan: ");
      Serial.print(ld2410.staticDistanceCm);
      Serial.print("cm (energia ");
      Serial.print(ld2410.staticEnergy);
      Serial.print(") | havaintoetaisyys: ");
      Serial.print(ld2410.detectionDistanceCm);
      Serial.print("cm");
    }
    Serial.println();
  }
}

// Lukee ja purkaa LD2410:n UART-datakehykset (ei ACK/asetuskehyksia)
void readLD2410() {
  while (LD2410Serial.available()) {
    uint8_t b = LD2410Serial.read();

    if (!inFrame) {
      // etsitaan kehyksen alkumerkkia
      if (frameLen < 4) {
        if (b == FRAME_HEADER[frameLen]) {
          frameBuf[frameLen++] = b;
          if (frameLen == 4) inFrame = true;
        } else {
          frameLen = (b == FRAME_HEADER[0]) ? 1 : 0;
          if (frameLen == 1) frameBuf[0] = b;
        }
      }
    } else {
      if (frameLen < sizeof(frameBuf)) {
        frameBuf[frameLen++] = b;
      }

      // tarkistetaan onko loppumerkki tullut
      if (frameLen >= 8) {
        bool footerMatch = true;
        for (int i = 0; i < 4; i++) {
          if (frameBuf[frameLen - 4 + i] != FRAME_FOOTER[i]) {
            footerMatch = false;
            break;
          }
        }
        if (footerMatch) {
          parseFrame(frameBuf, frameLen);
          inFrame = false;
          frameLen = 0;
        }
      }

      if (frameLen >= sizeof(frameBuf)) {
        // liian pitka kehys, aloitetaan alusta
        inFrame = false;
        frameLen = 0;
      }
    }
  }
}

// frame: F4 F3 F2 F1 | len_l len_h | data... | F8 F7 F6 F5
void parseFrame(uint8_t *frame, size_t len) {
  if (len < 12) return;

  uint16_t dataLen = frame[4] | (frame[5] << 8);
  uint8_t *data = &frame[6];

  if (dataLen < 1 || (size_t)(6 + dataLen + 4) > len) return;

  // Datatyyppi: 0x01 = engineering, 0x02 = normal
  uint8_t dataType = data[0];
  if (dataType != 0x02 && dataType != 0x01) return;

  // Normal target data alkaa headilla 0xAA
  if (data[1] != 0xAA) return;

  ld2410.targetState        = data[2];
  ld2410.movingDistanceCm   = data[3] | (data[4] << 8);
  ld2410.movingEnergy       = data[5];
  ld2410.staticDistanceCm   = data[6] | (data[7] << 8);
  ld2410.staticEnergy       = data[8];
  ld2410.detectionDistanceCm = data[9] | (data[10] << 8);
  ld2410.valid = true;
}
