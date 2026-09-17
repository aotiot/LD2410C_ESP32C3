HLK-LD2410C + ESP32-C3 mini
Arduino-sketch, joka lukee HLK-LD2410C-mmWave-tutkasensoria ESP32-C3 minillä ja tulostaa ihmisen läsnäolotiedon Serial Monitoriin.

Kytkentä
Sensorin pinni	Toiminto	Kytketään ESP32-C3:een
PIN1	UART TX	RX_PIN (ristiin)
PIN2	UART RX	TX_PIN (ristiin)
PIN3	OUT / human detected	PRESENCE_PIN
VCC	Käyttöjännite	5V tai 3.3V (moduulikohtainen)
GND	Maa	GND
Huom: UART-pinnit kytketään ristiin — sensorin TX menee ESP32:n RX:ään ja päinvastoin.

Asetukset sketchissä
Muuta LD2410C_ESP32C3.ino alussa olevat GPIO-numerot vastaamaan omaa kytkentää:

#define RX_PIN        4   // ESP32-C3 GPIO <- sensorin PIN1 (TX)
#define TX_PIN        5   // ESP32-C3 GPIO -> sensorin PIN2 (RX)
#define PRESENCE_PIN  6   // ESP32-C3 GPIO <- sensorin PIN3 (OUT)
Oletusarvot ovat esimerkkejä eivätkä välttämättä sovi kaikille ESP32-C3 mini -korteille (esim. GPIO2/8/9 ovat strapping-pinnejä ja niitä kannattaa välttää).

Miten sketch toimii
PIN3 (OUT): luetaan suoraan digitalRead-kutsulla. Tämä on sensorin oma yksinkertainen läsnäololähtö (HIGH = ihminen havaittu), joka toimii riippumatta UART-liikenteestä.
PIN1/PIN2 (UART): ESP32:n HardwareSerial(1) lukee sensorin datakehykset 256000 baudin nopeudella (LD2410C:n oletus). Sketch purkaa normaalit raportointikehykset (F4 F3 F2 F1 ... F8 F7 F6 F5) ja poimii:
kohteen tilan (ei kohdetta / liikkuva / paikallaan / molemmat)
liikkuvan kohteen etäisyyden (cm) ja energian
paikallaan olevan kohteen etäisyyden (cm) ja energian
kokonaishavaintoetäisyyden (cm)
Käyttö
Avaa LD2410C_ESP32C3.ino Arduino IDE:ssä.
Valitse levytyyppi "ESP32C3 Dev Module" (Boards Manager: esp32 by Espressif Systems).
Tarkista/muuta GPIO-määrittelyt kytkentää vastaaviksi.
Lataa sketch laitteelle.
Avaa Serial Monitor 115200 baudilla — tulostuu läsnäolotieto ja UART:sta puretut mittaustiedot 2x sekunnissa.
Riippuvuudet
Ei ulkoisia kirjastoja — sketch käyttää vain ESP32-ydinpaketin HardwareSerial-luokkaa. Vaatii asennetun "esp32 by Espressif Systems" -alustan Arduino IDE:hen.
