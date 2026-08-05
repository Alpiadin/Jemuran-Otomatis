#define BLYNK_TEMPLATE_ID "TMPL6bz58h8km"
#define BLYNK_TEMPLATE_NAME "Nipis Madu"
#define BLYNK_AUTH_TOKEN "APAoC6-UM27QCdQ8b9-fV_Z7l-51L7VH"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <AccelStepper.h>
#include <DHT.h>

char ssid[] = "qifatraa";
char pass[] = "qifatraa";

bool blynkConnected = false;

#define RAIN_SENSOR 15
#define LDR_PIN 34

#define DHTPIN 5
#define DHTTYPE DHT22

#define DIR_PIN 16
#define STEP_PIN 17

#define LED_MERAH 18
#define LED_HIJAU 19

#define LIMIT_TERBUKA 2
#define LIMIT_TERTUTUP 4

AccelStepper stepper(1, STEP_PIN, DIR_PIN);
DHT dht(DHTPIN, DHTTYPE);

bool statusHujanSebelumnya = false;
bool statusAtapTerbuka = true;

bool notifBerhasil = false;
bool notifGagal = false;

unsigned long waktuMulaiMenutup = 0;
bool prosesMenutup = false;

unsigned long waktuMulaiHujan = 0;
bool timerHujanAktif = false;

bool timerCerahAktif = false;
unsigned long waktuMulaiCerah = 0;

bool hujanValid = false;

const unsigned long DELAY_HUJAN = 3000;
unsigned long lastDHT = 0;

unsigned long lastLDR = 0;

String statusCahaya = "";
bool malam = false;

bool timerMalamAktif = false;
bool timerSiangAktif = false;

unsigned long waktuMulaiMalam = 0;
unsigned long waktuMulaiSiang = 0;

const unsigned long DELAY_LDR = 5000;

void setup() {

  Serial.begin(115200);
  dht.begin();

  pinMode(RAIN_SENSOR, INPUT_PULLUP);
  pinMode(LIMIT_TERBUKA, INPUT_PULLUP);
  pinMode(LIMIT_TERTUTUP, INPUT_PULLUP);

  pinMode(LDR_PIN, INPUT);

  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  stepper.setMaxSpeed(100);
  stepper.setAcceleration(100);

  Serial.println("Menghubungkan WiFi...");

  WiFi.begin(ssid, pass);

  unsigned long mulai = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - mulai < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println();
    Serial.println("WiFi Connected");

    Blynk.config(BLYNK_AUTH_TOKEN);

    if (Blynk.connect(5000)) {

      Serial.println("Blynk Connected");
      blynkConnected = true;

    } else {

      Serial.println("Blynk Gagal");
    }

  } else {

    Serial.println();
    Serial.println("WiFi Tidak Tersambung");
  }

  Serial.println("SmartDry Ready");
}

void loop() {

if (blynkConnected) {
  Blynk.run();
}

if (millis() - lastDHT >= 2000) {

  lastDHT = millis();

  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (!isnan(suhu) && !isnan(kelembapan)) {

    Serial.print("Suhu : ");
    Serial.print(suhu);
    Serial.println(" °C");

    Serial.print("Kelembapan : ");
    Serial.print(kelembapan);
    Serial.println(" %");

    if (blynkConnected) {

      Blynk.virtualWrite(V3, suhu);
      Blynk.virtualWrite(V4, kelembapan);

    }

  } else {

    Serial.println("DHT22 gagal dibaca");

  }

}

//============================
// SENSOR LDR
//============================
if (millis() - lastLDR >= 1000) {

  lastLDR = millis();

  int nilaiLDR = analogRead(LDR_PIN);

  Serial.print("ADC : ");
  Serial.println(nilaiLDR);

  // Konversi menjadi persen (0-100)
  int cahaya = map(nilaiLDR, 4095, 0, 0, 100);
  cahaya = constrain(cahaya, 0, 100);

  String statusBaru;

  if (cahaya >= 70) {

    statusBaru = "CERAH";

  }
  else if (cahaya >= 30) {

    statusBaru = "MENDUNG";

  }
  else {

    statusBaru = "MALAM";

  }

  Serial.print("LDR : ");
  Serial.print(cahaya);
  Serial.println("%");

  Serial.print("Status Cahaya : ");
  Serial.println(statusBaru);

  if (blynkConnected) {

    Blynk.virtualWrite(V5, cahaya);
    Blynk.virtualWrite(V8, statusBaru);

  }

  statusCahaya = statusBaru;

//============================
// DELAY DETEKSI MALAM
//============================

// Mulai timer saat pertama kali gelap
if (statusBaru == "MALAM" && !timerMalamAktif && !malam) {

  waktuMulaiMalam = millis();
  timerMalamAktif = true;

}

// Jika tetap gelap selama 5 detik
if (timerMalamAktif &&
    statusBaru == "MALAM" &&
    millis() - waktuMulaiMalam >= DELAY_LDR) {

  malam = true;

  timerMalamAktif = false;

  Serial.println("STATUS BERUBAH : MALAM");

}

// Kalau sebelum 5 detik terang lagi
if (statusBaru != "MALAM") {

  timerMalamAktif = false;

}

//============================
// DELAY DETEKSI SIANG
//============================

// Mulai timer saat terang kembali
if (statusBaru != "MALAM" && !timerSiangAktif && malam) {

  waktuMulaiSiang = millis();
  timerSiangAktif = true;

}

// Jika tetap terang selama 5 detik
if (timerSiangAktif &&
    statusBaru != "MALAM" &&
    millis() - waktuMulaiSiang >= DELAY_LDR) {

  malam = false;

  timerSiangAktif = false;

  Serial.println("STATUS BERUBAH : SIANG");

}

// Kalau sebelum 5 detik gelap lagi
if (statusBaru == "MALAM") {

  timerSiangAktif = false;

}

}

  bool hujan = digitalRead(RAIN_SENSOR) == LOW;

  //============================
  // HUJAN BARU TERDETEKSI
  //============================
if (hujan && !timerHujanAktif && !statusHujanSebelumnya) {

  waktuMulaiHujan = millis();
  timerHujanAktif = true;
}

//============================
// Jika hujan terus selama 3 detik
//============================
if (timerHujanAktif &&
    hujan &&
    millis() - waktuMulaiHujan >= DELAY_HUJAN &&
    !statusHujanSebelumnya) {

    hujanValid = true;

    timerCerahAktif = false;

  Serial.println("HUJAN TERDETEKSI");

if (blynkConnected) {
  Blynk.logEvent("hujan_terdeteksi");
}
  waktuMulaiMenutup = millis();
  prosesMenutup = true;

  notifBerhasil = false;
  notifGagal = false;

  statusHujanSebelumnya = true;

  timerHujanAktif = false;
    }

  //============================
  // CERAH KEMBALI
  //============================
// Sensor mulai kering
if (!hujan && !timerCerahAktif && hujanValid) {

  waktuMulaiCerah = millis();
  timerCerahAktif = true;
}

// Sensor tetap kering selama 3 detik
if (timerCerahAktif &&
    !hujan &&
    millis() - waktuMulaiCerah >= DELAY_HUJAN) {

  hujanValid = false;

  Serial.println("CUACA CERAH");

  statusHujanSebelumnya = false;

  timerCerahAktif = false;
}

// Kalau sebelum 3 detik hujan lagi
if (hujan && timerCerahAktif) {

  timerCerahAktif = false;
}
  //============================
  // LOGIKA ATAP
  //============================
  if (hujanValid || malam) {

    if (statusAtapTerbuka) {

if (hujanValid)
    Serial.println("ATAP MENUTUP (HUJAN)");
else
    Serial.println("ATAP MENUTUP (MALAM)");
  
      digitalWrite(LED_MERAH, HIGH);
      digitalWrite(LED_HIJAU, LOW);

if (blynkConnected) {
if (hujanValid)
    Blynk.virtualWrite(V0, "CUACA HUJAN");
else
  Blynk.virtualWrite(V0, "MALAM");
  Blynk.virtualWrite(V1, "ATAP TERTUTUP");
}
      statusAtapTerbuka = false;
    }

    stepper.moveTo(1700);

  } else {

    if (!statusAtapTerbuka) {

      Serial.println("ATAP MEMBUKA");

      digitalWrite(LED_MERAH, LOW);
      digitalWrite(LED_HIJAU, HIGH);

if (blynkConnected) {
  Blynk.virtualWrite(V0, "CUACA CERAH");
  Blynk.virtualWrite(V1, "ATAP TERBUKA");
}
      statusAtapTerbuka = true;
    }

    stepper.moveTo(0);
  }

  stepper.run();

  //============================
  // NOTIF BERHASIL
  //============================
  if (prosesMenutup &&
      digitalRead(LIMIT_TERTUTUP) == LOW &&
      !notifBerhasil) {

    Serial.println("ATAP BERHASIL DITUTUP");

if (blynkConnected) {
  Blynk.logEvent("atap_berhasil");
}
    notifBerhasil = true;
    prosesMenutup = false;
  }

  //============================
  // NOTIF GAGAL
  //============================
  if (prosesMenutup &&
      !notifGagal &&
      millis() - waktuMulaiMenutup > 10000) {

    Serial.println("ATAP GAGAL MENUTUP");

if (blynkConnected) {
  Blynk.logEvent("atap_gagal");
}
    notifGagal = true;
    prosesMenutup = false;
  }
}