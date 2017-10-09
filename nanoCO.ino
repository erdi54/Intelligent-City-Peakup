  #include <SoftwareSerial.h>
  SoftwareSerial COSerial(8, 7);


    
   int time_interval=8;
   void setTimerPWM(byte chA,byte chB)
   {
      TCCR2A = 0b10100011; //OCA,OCB, hızlı pwm
      TCCR2B = 0b001; //ön derecelendirici yok 
      OCR2A = chA; //0..255
      OCR2B = chB;
   }

   float optimum_voltage=0;
   byte optimum_width=240; //varsayılan makul değer

  //Aşağıdaki fonksiyon, çeşitli PWM döngüsü genişliklerini dener ve sonuç olarak
  // her girişim için voltaj değini arar , daha sonra en uygun olanını seçer
  // bu değer programda daha sonra kullanılır
   void set_pwm()
   {
       float prev_voltage=5.0; //önceki aramadaki volataj değeri
       float crude2V=5.0/1024.0;// Arduino yu dönüştürmek için katsayı 
       // Analog Voltajı , volataj döüşümü yapıyoruz 
       for(int i=0 ; i<250 ; i++)
       {
             setTimerPWM(0,i);
             float avg_v= 0;
             
             for(int j=0; j<100; j++ )
             {
               avg_v+= analogRead(A1);
               delay(time_interval);
              
              }
              avg_v *= 0.01;
              avg_v *= crude2V;
              Serial.print("Set PWM i=");
              Serial.print(i);
              Serial.print(", V=");
              Serial.println(avg_v);
              
              if(avg_v<3.6 && prev_voltage>3.6)
              {
                float dnew=3.6-avg_v; //Suanki değeri bulumamız için
                float dprev=prev_voltage-3.6;// veya önceki değer daha iyidir
              if(dnew < dprev)//Eğer yeni bir 1.4'e yakınsa o zaman geri getirin
              {
                optimum_voltage=avg_v;
                optimum_width=i;
                return;
              }
              else //yoksa bir önceki döner
              {
                 optimum_voltage=prev_voltage;
                 optimum_width=i-1;
                 return;  
              }
           }
             
            prev_voltage=avg_v;
        }
   }

  float ref_resistor_kOhm= 10.0;
   
  // ölçüm aşamasının sonunda (ısıtma başlamadan önce) temiz havada burada ham sensör değerini doldurun! Doğru hesaplama için bu kritik önem taşıyor
  float sensor_reading_clean_air = 660;
  
  //bu degeri ölçebilirsen
  // bazı CO sayacı veya tam olarak hesaplanmış CO örneği kullanarak, ardından doldurun
  // aksi durumda -1 bırakın, bu durumda varsayılan değerler kullanılacaktır
  float sensor_reading_100_ppm_CO = -1;
  float sensor_100ppm_CO_resistance_kOhm; // sensor_reading_100_ppm_CO değişkeninden hesaplandı
  float sensor_base_resistance_kOhm; // sensor_reading_100_ppm_CO değişkeninden hesaplandı
  
  
  byte phase=0; // 1 - yüksek voltaj, 0 - düşük voltaj, ölçmeden başlayalım
  
  
  unsigned long  previous_ms=0;//önceki m_saniye içindeki saykıl 
  unsigned long sec10 = 0; // bu zamanlayıcı saniyede 10 defa güncellenir
  
 
  unsigned long high_on = 0; // yüksek sıcaklık saykıl başladığımız zaman
  unsigned long low_on = 0; // düşük sıcaklık saykıl başladığımız zaman
  unsigned long last_print = 0;// mesajı seri olarak en son yazdırdığımız zaman
  
 
  float sensor_val=0; // mevcut sensör değeri
  float last_CO_ppm_value=0; // Önceki ölçüm saykıl sonunda CO konsantrasyonu
  

  float crude_value_CO_ppm(float value)
  {
      if(value<1) return -1;
    
         sensor_base_resistance_kOhm=ref_resistor_kOhm*1023/sensor_reading_clean_air - ref_resistor_kOhm;
        if(sensor_reading_100_ppm_CO >0)
        {
           sensor_100ppm_CO_resistance_kOhm = ref_resistor_kOhm * 1023 / sensor_reading_100_ppm_CO - ref_resistor_kOhm;
        }
        else
        {
           sensor_100ppm_CO_resistance_kOhm=sensor_base_resistance_kOhm*0.5;
        }
    
        float sensor_R_kOhm = ref_resistor_kOhm * 1023 /value - ref_resistor_kOhm;
        float R_relation = sensor_100ppm_CO_resistance_kOhm / sensor_R_kOhm;
        float CO_ppm=100*(exp(R_relation)-1.648);
    
        if(CO_ppm<0) CO_ppm=0;
        return CO_ppm;
   }

  void Start_Measurement()
  {
      phase=0;
      low_on = sec10;
      setTimerPWM(0, optimum_width);
      
  }

  void Start_Heating()
  { 
      phase = 1;
      high_on = sec10;
      setTimerPWM(0, 255);
  }

  void setup() {
  
   Serial.begin(9600);
   COSerial.begin(9600); 
  
    while(!Serial){//Seriport bekleniyor
       Serial.print("."); 
      }
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
    pinMode(3, OUTPUT);
    analogReference(DEFAULT);
   
  
    set_pwm();
    Serial.print("PWM sonucu:genislik");
    Serial.print(optimum_width);
    Serial.print(",volataj");
    Serial.print(optimum_voltage);
    Serial.println("Veri cikisi: Ham A0 degeri, isitma acik / kapali (0.1 kapali 1000.1 on), CO ppm, son ölcüm saykilinda");
    Start_Measurement();
  
  }
  
  void loop() {
    unsigned long ms = millis();
    int dt = ms - previous_ms;
  
    if(dt > 10*time_interval || dt < 0) 
    {
        previous_ms = ms; //Önceki çevrim süresini sakla
        sec10++; //artış 0.1s sayacı
    }
  
      if(phase == 1 && sec10 - high_on > 600)
      {
        Start_Measurement();  //60 saniye boyunca ısıtma bittimi kontrol ediyor ???
      } 
   
          
        if(phase == 0 && sec10 - low_on > 900) //90 saniye ölçüm sona erdi?
        {
          //last_CO_ppm_value = crude_value_CO_ppm(sensor_val);
           Start_Heating();
        }
  
        float v = analogRead(A0);
        sensor_val *= 0.999; //formül yardımıyla üssel ortalama uygulandı 
        sensor_val += 0.001*v; //ortalama = eski ortalama * a + (1-a) * yeni okuma
       
        if(sec10 - last_print > 9) //Ölçüm sonuçlarını saniyede 2 kez seri yazdırma
        { 
          last_print = sec10;
          Serial.print(sensor_val);
          Serial.print(" ");
          Serial.print(0.1 + phase*1000);
          Serial.print(" ");
         // Serial.println(last_CO_ppm_value);
          if(phase==0 )
          {
            last_CO_ppm_value = crude_value_CO_ppm(sensor_val);
            Serial.println(last_CO_ppm_value );
          
            int value;
            
            value=(int)last_CO_ppm_value ;
            Serial.println(value);
            COSerial.write(last_CO_ppm_value);
           
          }else
            {
              Serial.println("isinma evresi...");
            }
              
        }
  }
