#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <SoftwareSerial.h>

Adafruit_ADS1115 ads1115(0x48);

int TEC  = 3, DIR  = 8, SNSR = 0;
double temp, setpoint=300; 
int power, steps=0;
char inbyte;
int16_t adc0;
double kp = 5.0, kd = 0.0, ki = 1;
double output, temp_last= 305, tau=0.5;
boolean start = false;
const byte numChars = 32;
char receivedChars[numChars];
boolean newData = false;
char temporary[numChars];

double pid_calc(double kp, double kd, double ki, \
    double t, double tprev, double tset, \
    double tau, int steps){
  double terr = tset - t;
  double terr_prev = tset - tprev;
  double output;
  output = kp*terr + kd*(terr - terr_prev)/tau + ki*terr*tau*steps;
  return output;
}

void setup(){
	pinMode(TEC,OUTPUT);
	pinMode(DIR,OUTPUT); 
  ads1115.begin();
	Serial.begin(9600);
	delay(500);
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
    
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                    
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void loop(){
      newData=false;
      if(Serial.available()){
        recvWithStartEndMarkers();
        if(receivedChars[0] == 's' && receivedChars[1] == '_' && newData==true){
          strncpy(temporary,receivedChars+2,strlen(receivedChars)-2);
          setpoint = atof(temporary);
          Serial.println(temporary);
          start = true;
        }
        if(receivedChars[0] == 't' && receivedChars[1] == '_' && newData==true){  
          adc0 = ads1115.readADC_SingleEnded(0);
          temp = adc0*0.1875-11;          
          output = pid_calc(kp,kd,ki,temp,temp_last, setpoint, tau,steps);          
          if(output<-255){output = -255;}
          if(output>255){output = 255;}
          if(output<=0){digitalWrite(DIR,LOW);analogWrite(TEC,abs(output));}
          if(output>0){digitalWrite(DIR,HIGH);analogWrite(TEC,abs(output));}
          temp_last = temp;
          steps++;
          Serial.print(adc0*0.1875 -11, 4);
          Serial.print(" ");
          Serial.print(output);
          Serial.print("\n");    
        }
      }
  delay(500);
}

