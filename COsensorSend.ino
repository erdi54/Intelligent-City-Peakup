
   #include <SimpleZigBeeRadio.h>
   #include <SoftwareSerial.h>
   #include <Timer.h>
   
   Timer t;
   
   // xbee objesi oluştur
   SimpleZigBeeRadio Xbee = SimpleZigBeeRadio();
  
   //Seriport tanımlıyoruz
   //Not: Aynı anda tek seriport okuna bilir Buz yüzden xbee ye ayrı bir seriport tanımlıyoruz
   
   SoftwareSerial XBeeSerial(10, 11); 
   //CO Sensörü Seri portan PPM değeri okuyor
   SoftwareSerial COSerial(8, 7);

   ///////////////////////////////////////////////////////
   //ortalama işlemi değişkenleri 
      
    const int reading_num=39;//okunup toplanacak  değer sayısı
  
    int readings[reading_num]; //Seriporttan okunan Değerler
    int IndexRead=0; //mevcut okunan index
    int total=0;    //toplam
    int average=0;  //ortalama
    
 
   /////////////////////////////////////////////////////// 
    
    unsigned long time =0;
    unsigned long LastSent=0;
    int sum=0;
    int check=0;
    int value=0;
    int ID=19;
    int sayac=0;
   
    ///////////////////////////////////////////////////////////////////////////
    
 
    void setup() {
    Serial.begin(9600);
    XBeeSerial.begin(9600);
    COSerial.begin(9600);
    COSerial.setTimeout(10000);
   
    ///////////////////////////////////////////////////////////////////
   
    Xbee.setSerial(XBeeSerial);//Bee radyosunun seri bağlantı noktasını ayarlayın.
    Xbee.setAcknowledgement(true);
    //Radyonun API Mod 2'de olduğundan ve doğru PAN Kimliği üzerinde çalıştığından emin olmak için, AT Komutları AP'sini ve Kimliğini kullanabilirsiniz
    //Not: Bu değişiklikler uçucu  bellekte saklanır ve güç kesilirse devam etmeyecektir.
    Xbee.prepareATCommand('AP',0);
    Xbee.send();
    delay(200);
    uint8_t panID[] = {0x12,0x34}; //max 64bit
    Xbee.prepareATCommand('ID',panID,sizeof(panID));
    Xbee.send();
    
    //////////////////////////////////////////////////////////////////
    //Tüm okunana değerleri '0' dan başlat
    for(int ThisReading=0; ThisReading<reading_num; ThisReading++)
    {
       readings[ThisReading]=0;
    }
    //////////////////////////////////////////////////////////////////
    // Timer.h  interrupt Süresi (Veri yollama işlemine gidecek)/////
    ////////////////////////////////////////////////////////////////
     t.every(30000, Perform_Action);
    
  
    }

    void loop()
    {
     
     //int ID=1;
     //int ID=3;
     float v=COSerial.read();
     Serial.println(v);
    
  
     if( v== -1.00 )
     {sayac++;
     
          if(sayac>2){
          Serial.println("CO Sensoru kalibrasyonu gerceklesiyor");
         
          }
      }
      else
      { sayac=0;
             
            //SERİPORTTAN GELEN DEĞERLERİ DÜZETME ALGORİTMASINA TABİ TUTUYORUZ 
            //BUNU YAPMAKTAKİ AMACIMIZ GELEN VERİLERİN HAREKETLİ ORTALAMASINI ALMAK
            //GÖNDERİLEN DEĞERLERİ OLABİLDİĞİNCE STABİL OLARAK GÖNDERMEK 
             
            // son okunan değeri çıkart 
            total=total-readings[IndexRead];
            value=(int)v ;
            Serial.println(value);
            //Sensör Değeri
            readings[IndexRead]=value;
            //okunan değeri toplama ekle
            total = total + readings[IndexRead];
            // dizideki bir sonraki konuma ilerledik 
            IndexRead=IndexRead+1;
            //Eger dizinin sonuna gelmişse
            if(IndexRead>=reading_num){ 
            //başlangıç index'se dön
              IndexRead=0;   
             }
             
             average = total /reading_num;
             Serial.println("////////////////////////////");
             Serial.print("Ortalama:");
             Serial.println(average);
             Serial.println("////////////////////////////");
             delay(100);
        
            //Belirlenen Süreden sonra "Perform_Action" fonsiyonuna gidiyor  
            //Bu belirlenen sürede diğer işlemler devam eder...  
             t.update();
             
        }
  
        delay(500);
    }
  
    
    void XBeeDataSend(int value, int ID)
    {  
      while(Xbee.available())
      {
        // Veriyi oku
        Xbee.read();
       //Eğer eksiksiz bir mesaj varsa, içeriğini kontrol et
        if(Xbee.isComplete())
        {
        Serial.print("\nGelen Mesaj: ");
        Packetprint(Xbee.getIncomingPacketObject());
    
          if(Xbee.isRX() )
           {
                Serial.println("Alınan RX Paketi");
                if( Xbee.getRXPayloadLength() == 2 ){
                    int payloadVal = (Xbee.getRXPayload(0) << 8) + Xbee.getRXPayload(1);
          
                    Serial.print( "Alınan Sum: " );
                    Serial.println( payloadVal );
                    Serial.print( "Saklanmıs Sum: " );
                    Serial.println( sum );
                 }
         
         
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
              }
            else{
              // Diğer veya uygulanmamış Frame türü
              Serial.println( "Diger Frame Tipi" );
            }
         }
      }
      
           time=millis();
           if( time > (LastSent+5000))
           { 
             LastSent=time;//Değikeni güncelleme süresi
             uint8_t payload[3];
             payload[0] = ID & 0xff;
            ////////////////////////////
            ////////////////////////////
             payload[1] = value >> 8 & 0xff;
             payload[2] = value & 0xff;
             
             Xbee.prepareTXRequestToCoordinator( payload, sizeof(payload) );
             Xbee.send();
             Serial.println();
             Serial.print( "Gonderilen paket: " );
             Packetprint( Xbee.getOutgoingPacketObject() );
           }
    
          delay(10); // Kararlılık için küçük gecikme
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
        
        }
      /////////////////////////////////////////////////////////
      // Perform_Action fonkusyonu tanımlanan sürede veriyi-//
      //-yollama işlemini gerçekleştirecek                 // 
      //////////////////////////////////////////////////////
       void Perform_Action()
       {
          Serial.println("Veri yollanıyor");
           XBeeDataSend( average, ID);
        
       } 

