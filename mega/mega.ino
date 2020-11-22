  
//arduino-cli compile  --fqbn arduino:avr:mega 
//arduino-cli upload -p /dev/cu.usbserial-14140  --fqbn arduino:avr:mega

#include <ClickEncoder.h>
#include <TimerOne.h>
#include "ColorConverterLib.h"
#define LOG Serial
#include "log.h"

struct encoderSetup {
    double encoder_last, encoder_value;
    char encoder_software_state[3] = "HSL";
    int encoder_software_state_index = 0;
    ClickEncoder *encoder;
    int HSLlmits[3] = {360, 100, 100};
} encoderConfig;

struct lampSetup {
    double H=0;
    double S=100;
    double L=50;
    double H_prev;
    double S_prev;
    double L_prev;
    char working_mode[15]="basic";
} lampConfig;

struct pinSetup {
    int RED = 3;  
    int GREEN = 5;  
    int BLUE = 4;  
    int FAN_PWM = 8;
    int FAN_RPM = 9;
    const int ENCODER_BUTTON = 49;
    const int ENCODER_A = 50;
    const int ENCODER_B = 48;
} pinConfig;


void hexToRgb(unsigned long hexValue, uint8_t& r, uint8_t& g, uint8_t& b) {

    r = hexValue >> 16;
    g = (hexValue & 0x00ff00) >> 8;
    b = (hexValue & 0x0000ff);
}

void setColorFromHexString(char *hexString) {

    uint8_t r;
    uint8_t g;
    uint8_t b;
    double H,S,L;
    
    unsigned long hexValue = (unsigned long)strtol(hexString, NULL, 16);
    hexToRgb(hexValue,r,g,b);
    setRgb(r,g,b);

    ColorConverter::RgbToHsl(r,g,b, H,S,L);
    lampConfig.H = H*360;
    lampConfig.S = S*100;
    lampConfig.L = L*100;	
    //Serial.println(encoderConfig.encoder_software_state_index);
    switch (encoderConfig.encoder_software_state_index) {
        case 0:
            encoderConfig.encoder_value  = lampConfig.H;
        break;
        case 1:
            encoderConfig.encoder_value = lampConfig.S;
        break;
        case 2:
            encoderConfig.encoder_value = lampConfig.L;
        break;
    }   

}

int get_fan_rpm() {

  unsigned long pulseDuration;
  pulseDuration = pulseIn(pinConfig.FAN_RPM, LOW);
  double frequency = 1000000/pulseDuration;

  return (int)(frequency/2*60)/2;
}

bool startsWith(const char *a, const char *b) {
    return (strncmp(a, b, strlen(b)) == 0) ? true : false;   
}

void serial_read() {

  if (Serial3.available() > 0) {   

    String espString;
    const char *espStringP;
    
    espString = Serial3.readStringUntil('\n');
    espStringP = espString.c_str();

    if (startsWith(espStringP, "0x")) {
        setColorFromHexString(espStringP);
    }
    else 
        strcpy(lampConfig.working_mode, espStringP); 
  }
   
}

void initial_led_test() {

    int colorTime=800;

    setRgb(15,0,0);
    delay(colorTime);
    setRgb(0,15,0);
    delay(colorTime);
    setRgb(0,0,15);
    delay(colorTime);
    setRgb(0,0,0);

}

void analogWrite25k(int value){
    OCR4C = value; //correspond to pin 8
}

void setPin8to25k() {
    TCCR4A = 0;
    TCCR4B = 0;
    TCNT4  = 0;

    // Mode 10: phase correct PWM with ICR4 as Top (= F_CPU/2/25000)
    // OC4C as Non-Inverted PWM output
    ICR4   = (F_CPU/25000)/2;
    OCR4C  = ICR4/2;                    // default: about 50:50
    TCCR4A = _BV(COM4C1) | _BV(WGM41);
    TCCR4B = _BV(WGM43) | _BV(CS40);

}

void timerIsr() {
  encoderConfig.encoder->service();
}

void encoderInit() {
    encoderConfig.encoder = new ClickEncoder(pinConfig.ENCODER_A, pinConfig.ENCODER_B, pinConfig.ENCODER_BUTTON);
    Timer1.initialize(1000);
    Timer1.attachInterrupt(timerIsr);
    encoderConfig.encoder_last = -1;
}

void setup() {

    //setPin8to25k();    //Enable if the fan make noise (mostly in the middle range)

    Serial.begin(115200);
    Serial3.begin(115200);
    
    pinMode(pinConfig.RED, OUTPUT);  
    pinMode(pinConfig.GREEN, OUTPUT);  
    pinMode(pinConfig.BLUE, OUTPUT);  
    pinMode(pinConfig.FAN_PWM, OUTPUT);  
    pinMode(pinConfig.FAN_RPM, INPUT);

    initial_led_test();
    encoderInit();
    analogWrite(pinConfig.FAN_PWM,255); 
  
}

void setRgb(int r, int g, int b) {

    //int redBr = map(r, 0, 255, 0, 80); // temporary needed not to burn the led
    //log_printf("R:%i(%i) G:%i(%i) B:%i(%i)\n", redBr, 255-redBr,greenBr, 255-greenBr, blueBr,255-blueBr);

    analogWrite(pinConfig.RED, 255-r);
    analogWrite(pinConfig.GREEN, 255-g);
    analogWrite(pinConfig.BLUE, 255-b);

    if (r == 0 && g ==0 && b == 0) 
        //analogWrite25k(0); //enable only if setPin8to25k() is called
        analogWrite(pinConfig.FAN_PWM,0);
    else
        //analogWrite25k(300);
        analogWrite(pinConfig.FAN_PWM,255);  

}

 void setHsl(double _H, double _S, double _L) {

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    lampConfig.H = _H;
    lampConfig.S = _S;
    lampConfig.L = _L;

    ColorConverter::HslToRgb(lampConfig.H/360, lampConfig.S/100, lampConfig.L/100, red,green,blue);
    setRgb(red,green,blue);
  
 }

void workingModeLoop() {

    if (strcmp(lampConfig.working_mode,"disco") == 0)
        disco(100);
    else if (strcmp(lampConfig.working_mode,"strobo") == 0)
        strobo(65);
}

void encoderLoop() {

    ClickEncoder::Button encbut = encoderConfig.encoder->getButton();
    //Serial.print("Button: ");Serial.println(b);
    if (encbut != ClickEncoder::Open) {
        //Serial.print("Button: ");
        //Serial.print(encbut);
        switch (encbut) {
            case ClickEncoder::Clicked:
                encoderConfig.encoder_software_state_index++;
                if (encoderConfig.encoder_software_state_index > sizeof(encoderConfig.encoder_software_state)-1)
                    encoderConfig.encoder_software_state_index = 0;

                encoderConfig.encoder_value = encoderConfig.encoder_software_state[encoderConfig.encoder_software_state_index];
            break;
        }
    }

    encoderConfig.encoder_value += encoderConfig.encoder->getValue();
    if (encoderConfig.encoder_value != encoderConfig.encoder_last) {
        encoderConfig.encoder_last = encoderConfig.encoder_value;
        //Serial.print("Encoder Value: ");
        //Serial.println(encoderConfig.encoder_value);

        if (encoderConfig.encoder_value > encoderConfig.HSLlmits[encoderConfig.encoder_software_state_index])
            encoderConfig.encoder_value = 0;
        else if(encoderConfig.encoder_value < 0)
            encoderConfig.encoder_value = encoderConfig.HSLlmits[encoderConfig.encoder_software_state_index];
             
        switch (encoderConfig.encoder_software_state_index) {
            case 0:
                lampConfig.H = encoderConfig.encoder_value;
            break;
            case 1:
                lampConfig.S = encoderConfig.encoder_value;
            break;
            case 2:
                lampConfig.L = encoderConfig.encoder_value;
            break;
        }    
        // Serial.print("H: ");Serial.print(lampConfig.H);
        // Serial.print(" - S: ");Serial.print(lampConfig.S);
        // Serial.print("- L: ");Serial.print(lampConfig.L);
        // Serial.println();
        setHsl(lampConfig.H ,lampConfig.S ,lampConfig.L );
    }
    

}
 
void loop() {

  serial_read();
  workingModeLoop();
  encoderLoop();
     
}