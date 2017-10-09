  #include <Ethernet.h>
  #include <SPI.h>
  #include <SimpleZigBeeRadio.h>
  #include <SoftwareSerial.h>

  byte mac[] = { 0x90, 0xA2, 0xAD, 0x0F, 0xB9, 0xEC }; //Replace with your Ethernet shield MAC
  EthernetClient client;
  const char* server ="fatihwebapp.azurewebsites.net";
  /////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////
   
  // xbee objesi oluştur
  SimpleZigBeeRadio Xbee = SimpleZigBeeRadio();
  
  //Seriport tanımlıyoruz
  //Not: Aynı anda tek seriport okuna bilir Buz yüzden xbee ye ayrı bir seriport tanımlıyoruz
   SoftwareSerial XBeeSerial(8, 7); // 
    
   int lowByte=0;
   int highByte=0;
   int data=0;

   //cloud'a gönderilecek değişlenlerin adı 
   int sensor_value;
   int product_id;
  

  void setup()
  {
      Serial.begin(9600);
      while(!Serial)
       {
           //Seriport bekleniyor
           Serial.print("."); 
       }
      
     Serial.println("connecting...");
     
     ////////////////////////////////////////////////
     ////////////////////////////////////////////////////
     Ethernet.begin(mac);
     XBeeSerial.begin(9600);
     
     ///////////////////////////////////////
     Xbee.setSerial(XBeeSerial);//Bee radyosunun seri bağlantı noktasını ayarlayın.
     Xbee.setAcknowledgement(true);
     //Radyonun API Mod 2'de olduğundan ve doğru PAN Kimliği üzerinde çalıştığından emin olmak için, AT Komutları AP'sini ve Kimliğini kullanabilirsiniz
     //Not: Bu değişiklikler uçucu  bellekte saklanır ve güç kesilirse devam etmeyecektir.
     Xbee.prepareATCommand('AP',0);
     Xbee.send();
     Serial.print("burayim");
     delay(200);
     
     uint8_t panID[] = {0x12,0x34}; //max 64bit
     Xbee.prepareATCommand('ID',panID,sizeof(panID));
     Xbee.send();
      
   }
   
  void loop(){
     while(Xbee.available())
     {
      // Veriyi oku
      Xbee.read();
      Serial.println("Xbee read");
      delay(500);
     //Eğer eksiksiz bir mesaj varsa, içeriğini kontrol et
     
     if(Xbee.isComplete())
     {
        Serial.print("\nGelen Mesaj: ");
        Packetprint(Xbee.getIncomingPacketObject());
    
        if(Xbee.isRX())
        {
             Serial.println("Alınan RX Paketi");
             Serial.print( "Sum: " );
             int sum = 0;
                
                for(int i=0;i<Xbee.getRXPayloadLength();i++){
                sum = sum + Xbee.getRXPayload(i);
                 
                 if(i!=0){
                     Serial.print( " + " );
                   }
                  Serial.print(Xbee.getRXPayload(i));
                  
                }
      
                Serial.print( " = " );
                Serial.println( sum );
    
             
       int connect=5;
       // sum değeri bir int değerindir ve  böülünmesi gerekir 
       // Bu yüzden 2 byte a bölünmelidir
       uint8_t payload[2]; // int 2 byte 
            
         payload[0] = connect>> 8 & 0xff;
         payload[1] = connect & 0xff;
              
     // Gönderen adresiyle bir TX Paket hazırlayın.
     Xbee.prepareTXRequest(Xbee.getRXAddress(),payload,2);
              
     //xbee.prepareTXRequest(xbee.getRXAddress64().getAddressMSB(),xbee.getRXAddress64().getAddressLSB(),xbee.getRXAddress16().getAddress(),payload,2);
              
     Xbee.send();
     Serial.print( "Send: " );
     Packetprint( Xbee.getOutgoingPacketObject() );
    
              
    }else if(Xbee.isTXStatus() ){
              Serial.println( "Alinan TX Durumu" );
              Serial.print( "Durum: " );
              Serial.println(Xbee.getTXStatusDeliveryStatus(),HEX);
      }else if(Xbee.isATResponse()){
              Serial.println( "AT Komut Tepki Alinir" );
              Serial.print( "Durum: " );
              Serial.println(Xbee.getATResponseStatus(),HEX);
      }else if(Xbee.isRemoteATResponse()){
              Serial.println( "Uzaktan AT Komut Tepki Alindi" );
              Serial.print( "Durum: " );
              Serial.println(Xbee.getATResponseStatus(),HEX);
      }else if( Xbee.isModemStatus() ){
              Serial.println( "Alinan Modem Durumu" );
              Serial.print( "Durum: " );
              Serial.println(Xbee.getModemStatus(),HEX);
      }else{
              // Diğer veya uygulanmamış Frame türü
              Serial.println( "Diger Frame Tipi" );
           }
       }
    }
    
    delay(10);
    
  }

   /////////////////////////////////////////////////////////////
   // Bu fonksiyon tüm göderilen paketin içeriğini yazdırır   //
  /////////////////////////////////////////////////////////////
 
  void  Packetprint(SimpleZigBeePacket & p){
    Serial.print( START, DEC);
    Serial.print(' ');
    Serial.print( p.getLengthMSB(), DEC );
    Serial.print(' ');
    Serial.print( p.getLengthLSB(), DEC );
    Serial.print(' ');
    // Frame türü ve  Frame ID ve  Frame Verilerini saklanır
    uint8_t checksum = 0;
    
     for( int i=0; i<p.getFrameLength(); i++){
            
            Serial.print( p.getFrameData(i), DEC );
            Serial.print(' ');
            checksum += p.getFrameData(i); 
            }
    
   
    checksum = 0xff - checksum;
    Serial.print(checksum, DEC );
    Serial.println();

     
    Serial.println( p.getFrameData(14));
    Serial.println( p.getFrameData(15));
   product_id = p.getFrameData(14);
   sensor_value= p.getFrameData(15);

   Post_Method( product_id, sensor_value);
  
  }

/////////////////////////////////////////////////////////////
////////////// Ethernet Shild Json Post Metodu//////////////
///////////////////////////////////////////////////////////
void Post_Method(int product_id, int sensor_value)
{
      Serial.println("Veriler yollaniyor ");
     
    //String Dönüşümü json verisi oluşturma 
    
     String data = "{"
      "\"sensor_value\": \"" + String(sensor_value) + "\","
      "\"product_id\": \"" + String(product_id) + "\"}";
        
      
      if (client.connect(server,80)) {
      
      Serial.println("connected");
      client.println("POST /api/product/add_measurement.php  HTTP/1.1");
      client.println("Host:fatihwebapp.azurewebsites.net");
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(data.length());
      client.println();
      client.print(data);
      client.println();
       
      // Hata ayıklama için yayın isteğinizi yazdırır
      Serial.println("POST /add_measurement.php  HTTP/1.1");
      Serial.println("Host:fatihwebapp.azurewebsites.net");
      Serial.println("Content-Type: application/json");
      Serial.println("Connection: close");
      Serial.print("Content-Length: ");
      Serial.println(data.length());
      Serial.println();
      Serial.print(data);
      Serial.println();
      }
      delay(2000);
       
      if (client.connected()) {
      Serial.println();
      Serial.println("disconnecting.");
      client.stop();
      }
  
  }
