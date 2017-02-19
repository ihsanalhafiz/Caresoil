//Caresoil Code from Soilama UGM

#define WL1       0
#define WL2       1
#define SM1       2
#define SM2       3
#define SM3       4
#define PH        5
#define TRUE      1
#define FALSE     2
#define pompa     7
int readWL1,readWL2,readSM1,readSM2,readSM3,readPH,Nyala=0;
double level1,level2,lembab1,lembab2,lembab3,asamBasa;
int MINSM=0, MAXSM=950;

#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#define Wido_IRQ   7
#define Wido_VBAT  5
#define Wido_CS    10
#include "utility/debug.h"

Adafruit_CC3000 Caresoil = Adafruit_CC3000(Wido_CS, Wido_IRQ, Wido_VBAT,SPI_CLOCK_DIVIDER);

//Definisi Koneksi WIFI
#define WLAN_SECURITY   WLAN_SEC_WPA2
#define WLAN_SSID       "halo"         
#define WLAN_PASS       "123456789"

#define TCP_TIMEOUT      3000

#define TOKEN "8iqyaW3k83ixgUtlniENMxgTxBvgM3" //Token UBIDOTS

#define Var_ID_1 "58232a5d76254217be5be74f"  //pH Tanah
#define Var_ID_2 "58232a6a7625421861819b5b"  //Kelembaban Tanah
#define Var_ID_3 "58232a757625421861819b83"  //Ketinggian Air Lahan
#define Var_ID_4 "58232a7f7625421861819bcf"  //Ketinggian Air Sumber

float phtanah;
int kelembabantanah;
float airlahan;
float airsumber;
uint32_t ip = 0;    // Ubidots ip address
Adafruit_CC3000_Client CaresoilClient;

void setup() {
  
  Serial.begin(115200);
  Serial.println("Inisialisasi Caresoil");
  if (!Caresoil.begin())
  {
    Serial.println(F("Gagal inisialisasi Caresoil, mohon periksa koneksi dan wiring sistem"));
    while(1);
  }

  char *ssid = WLAN_SSID;
  Serial.print(F("\nMenghubungkan ke Internet Via: ")); 
  Serial.println(ssid);

  if (!Caresoil.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Koneksi gagal, periksa kembali sambungan internet!"));
    while(1);
  }

  Serial.println(F("Caresoil terhubung ke Internet!"));

  Serial.println(F("Request DHCP"));
  while (!Caresoil.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  } 
}

void loop() {
  WaterLevel1();
  WaterLevel2();
  SoilMoisture1();
  SoilMoisture2();
  double rataLembab = (lembab1+lembab2)/2.0 ;
  SensorPH();
  static unsigned long RetryMillis = 0;
  static unsigned long uploadtStamp = 0;
  static unsigned long sensortStamp = 0;
  

  if(!CaresoilClient.connected() && millis() - RetryMillis > TCP_TIMEOUT){
        // Update the time stamp for reconnecting the ip 
    RetryMillis = millis();

    Serial.println(F("Mencoba menghubungkan ke Server induk..."));

    ip = Caresoil.IP2U32(50,23,124,68);               
    CaresoilClient = Caresoil.connectTCP(ip, 80);         
    Serial.println(F("Caresoil berhasil terhubung ke Server Ubidots"));
  }

  if(CaresoilClient.connected() && millis() - uploadtStamp > 1000){
     // If the device is connected to the cloud server, upload the data every 180000ms.
     uploadtStamp = millis();

     // Mengirim Stream Data ke Ubidots
     kirimdata(Var_ID_1, String(asamBasa), Var_ID_2, String(rataLembab), Var_ID_3, String(level1), Var_ID_4, String(level2)); 
     
     // Mendapatkan respon dari server
     unsigned long rTimer = millis();
     Serial.println(F("Membaca Respon Sensor...\r\n"));
     while (millis() - rTimer < 2000) {
        while (CaresoilClient.connected() && CaresoilClient.available()) {
          char c = CaresoilClient.read();
          Serial.print(c);
        }
     }
     delay(1000);             // Wait for 1s to finish posting the data stream   

     CaresoilClient.close();      // Close the service connection
     RetryMillis = millis();  // Reset the timer stamp for applying the connection with the service
  }
}

//Fungsi Kirim data
void kirimdata(String var1, String val1, String var2, String val2, String var3, String val3, String var4, String val4){

   
    String httpBodyPackage = "[{\"variable\":\"" + var1 + "\",\"value\":" + val1 + "}, {\"variable\":\"" + var2 + "\",\"value\":" + val2 + "}, {\"variable\":\"" + var3 + "\",\"value\":" + val3 + "}, {\"variable\":\"" + var4 + "\",\"value\":" + val4 + "}]";
    Serial.println(httpBodyPackage);                       // Debug the http body stream

    //Make an HTTP request to the Ubidots server
    Serial.print(F("Sending Http Request..."));
    CaresoilClient.fastrprintln(F("POST /api/v1.6/collections/values/ HTTP/1.1"));
    CaresoilClient.fastrprintln(F("Host: things.ubidots.com"));
    CaresoilClient.fastrprint(F("X-Auth-Token: "));
    CaresoilClient.fastrprintln(TOKEN);
    CaresoilClient.fastrprintln(F("Content-Type: application/json"));
    CaresoilClient.fastrprint(F("Content-Length: "));
    CaresoilClient.println(String(httpBodyPackage.length()));
    CaresoilClient.fastrprintln(F(""));
    CaresoilClient.println(httpBodyPackage);

    Serial.println(F("Selesai....."));
}

float WaterLevel1(){
  readWL1 = analogRead(WL1);
  float level1 = 8*pow(10,-11)*pow(readWL1,4.1409);  
  return level1;
}


//Mengukur ketinggian air dengan sensor2
float WaterLevel2(){
  readWL2 = analogRead(WL1);
  float level2 = 8*pow(10,-11)*pow(readWL2,4.1409);  
  return level2;
}


//Mengukur kelembapan dengan sensor1
float SoilMoisture1(){
 readSM1 = analogRead(SM1);
 readSM1 = constrain(readSM1,308,1023);
 lembab1= map(readSM1,308,1023,100,0);
 return lembab1;
}


//Mengukur kelembapan dengan sensor2
float SoilMoisture2(){
  readSM2 = analogRead(SM2);
  readSM2 = constrain(readSM2,308,1023);
  lembab2= map(readSM2,308,1023,100,0);
  return lembab2;
}


//Mengukur kelembapan dengan sensor3
/*float SoilMoisture3(){
    readSM3 = analogRead(SM3);
    lembab3 =((readSM2-MINSM)/(MAXSM-MINSM))*100.0;
    return lembab3;
}
*/


//Mengukur PH
float SensorPH(){
  readPH = analogRead(PH);
  int convert;
  if (readPH>=0){
    convert = map(readPH,0,1023,0,420);
    asamBasa = ( convert/420.0)*7;
    }
  if(readPH<0){
    int absolut = (-1)*PH;
    convert = map(absolut,0,1023,0,420);
    asamBasa = ( convert/420.0)*7;
    }
    return asamBasa;
}

//mengaktifkan Pompa
// Nyala adalah parameter untuk mengaktifkan pompa
/*int aktuator(Nyala){
  if (Nyala==TRUE){
    digitalWrite(pompa,LOW);
    delay (5000);
  }
  else{
    digitalWrite(pompa,LOW);
  }
  
}
*/

//Parameter Tanaman
/*int padi(){
  if(33<=rataLembab && rataLembab<=90){
    Nyala = 1; 
  }
  else{
    Nyala = 0;
  }
  return Nyala;
}
*/

