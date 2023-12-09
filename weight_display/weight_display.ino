// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       weight_display.ino
    Created:	28/11/2023 7:36:02 μμ
    Author:     ROUSIS_FACTORY\user
*/
#define PIXELS_X 48
#define PIXELS_Y 16
#define CHAR_PER_LINE 8
#define TOTAL_CHAR 16
#define PIXELS_ACROSS 128      //pixels across x axis (base 2 size expected)
#define PIXELS_DOWN	16      //pixels down y axis
#define DRIVER_PIN_EN 26
#define PHOTO_SAMPLES 60
#define PHOTO_SAMPLE_DELAY 1000
#define LED 13

#define FLR_TRAFFIC 0
#define FLR_LEFT_SLIDE_SPEED 32
#define FLR_SLIDE_STEPS 21

#include <RousisMatrix16.h>
#include <fonts/SystemFont5x7_greek.h>

const int photoPin = 39;
// variable for storing the potentiometer value
uint8_t photoValue = 0;
uint8_t brightness = 255;
uint8_t sample_metter = 0;
uint8_t brigthsamples[PHOTO_SAMPLES];
bool bright_sens = true;

const char Company1[] = { "Rousis  " };
const char Company2[] = { "Systems " };
const char Device1[] = { "Weight  " };
const char Device2[] = { "Display " };
const char Version1[] = { "2X8 Char" };
const char Version2[] = { "V.1.1   " };
const char Init_start[] = { "Ready.." };

static char  receive_packet[32] = { 0 };
static uint8_t count = 0;
unsigned long time_delay = 0;

//RousisMatrix16 myLED(PIXELS_X, PIXELS_Y, 12, 14, 27, 26, 25, 33); 
RousisMatrix16 myLED(PIXELS_X, PIXELS_Y, 12, 14, 27, 26, 25, 33);

#define RXD2 16
#define TXD2 17
#define RS485_PIN_DIR 5
HardwareSerial rs485(1);
#define RS485_WRITE     1
#define RS485_READ      0
//-----------------------------------------------------------------------------------------------

hw_timer_t* timer = NULL;
hw_timer_t* flash_timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE falshMux = portMUX_INITIALIZER_UNLOCKED;
// Code with critica section
void IRAM_ATTR onTime() {
    portENTER_CRITICAL_ISR(&timerMux);
    myLED.scanDisplay();
    //digitalWrite (LED_PIN, !digitalRead(LED_PIN)) ;
    
    portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR FlashInt()
{
    portENTER_CRITICAL_ISR(&falshMux);
    //Photo_sample();

    portEXIT_CRITICAL_ISR(&falshMux);

}


void setup()
{
    Serial.begin(115200);
    rs485.begin(9600, SERIAL_8N1, RXD2, TXD2);
    pinMode(RS485_PIN_DIR, OUTPUT);
    digitalWrite(RS485_PIN_DIR, RS485_READ);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    delay(100);

    myLED.displayEnable();     // This command has no effect if you aren't using OE pin
    myLED.selectFont(SystemFont5x7_greek); //font1

    timer = timerBegin(0, 20, true);
    timerAttachInterrupt(timer, &onTime, true);
    Serial.println("Initialize LED matrix display");
    // Sets an alarm to sound every second
    timerAlarmWrite(timer, 10000, true);
    timerAlarmEnable(timer);

    uint8_t cpuClock = ESP.getCpuFreqMHz();
    flash_timer = timerBegin(1, cpuClock, true);
    timerAttachInterrupt(flash_timer, &FlashInt, true);
    timerAlarmWrite(flash_timer, 10000, true);
    //timerAlarmEnable(flash_timer);

    //-------------------------------------------------
    // Auto Brightness
    //pinMode(DRIVER_PIN_EN, OUTPUT);
    ////digitalWrite(DRIVER_PIN_EN, LOW);
    //// Ορίστε τον ακροδέκτη ως έξοδο auto brightness
    //// Ρύθμιση της συχνότητας του PWM
    //ledcSetup(0, 200, 8); // 200 Hz, ανάλυση 8 bits
    //// Σύνδεση του καναλιού PWM με τον ακροδέκτη
    //ledcAttachPin(26, 0);
    //delay(100);
    //ledcWrite(0, 128); // Παράγει ημιτονικό (50%) σήμα PWM
    //-------------------------------------------------

    Serial.print("Readed Startup Sample: ");
    Serial.println(analogRead(photoPin));
    photoValue = (255 / 4095.0) * analogRead(photoPin);
    brightness = 255 - photoValue;
    if (!brightness) { brightness = 10; }
    myLED.displayBrightness(brightness);
    Serial.print("First sample brightness: ");
    Serial.println(brightness);

    myLED.clearDisplay();
    myLED.drawFilledBox(0, 0, PIXELS_X - 1, 15, GRAPHICS_ON);
    delay(2000);
    myLED.clearDisplay();
    myLED.drawString(0, 0, Company1, CHAR_PER_LINE, 1);
    myLED.drawString(0, 9, Company2, CHAR_PER_LINE, 1);
    delay(1000);
    myLED.clearDisplay();
    myLED.drawString(0, 0, Version1, CHAR_PER_LINE, 1);
    myLED.drawString(0, 9, Version2, CHAR_PER_LINE, 1);
    delay(1000);
    myLED.clearDisplay();
    myLED.drawString(0, 9, Init_start, sizeof(Init_start), 1);


    Serial.println("Finished Initilising");
    time_delay = millis();
}

// Add the main program code into the continuous loop() function
void loop()
{
    if (rs485.available())
    {
        byte get_byte = rs485.read();
        if (get_byte == 0x0d) // this means carrier return (end of received message
        {
            size_t i;
            count = 0;
            char buf1[9]; char buf2[9];
            memcpy(buf1, receive_packet, CHAR_PER_LINE);
            memcpy(buf2, receive_packet + CHAR_PER_LINE, CHAR_PER_LINE);
            for (i = 0; i < CHAR_PER_LINE; i++)
            {
                if (buf1[i] < 0x20)
                    buf1[i] = ' ';
                if (buf2[i] < 0x20)
                    buf2[i] = ' ';
            }
            buf1[8] = 0; buf2[8] = 0;
            //myLED.clearDisplay();
            myLED.drawString(0, 0, buf1, CHAR_PER_LINE, 1);
            myLED.drawString(0, 9, buf2, CHAR_PER_LINE, 1);            
            
            Serial.println("Received packet: "); 
            Serial.println(receive_packet);
            Serial.println("______________________________");
            for (i = 0; i < TOTAL_CHAR; i++)
            {
                receive_packet[i] = ' ';
            }
        }
        else if (count > TOTAL_CHAR)
        {
            count = 0;
        }
        else {
            receive_packet[count++] = get_byte;
        }
    }

    if ((millis() - time_delay) > PHOTO_SAMPLE_DELAY)
    {
        Photo_sample();
        int state = digitalRead(LED);
        digitalWrite(LED, !state);
        time_delay = millis();
    }
}

void Photo_sample() {
    if (!bright_sens)
    {
        return;
    }
    if (sample_metter < PHOTO_SAMPLES)
    {
        //uint16_t val_sens = analogRead(photoPin);
        ////Serial.print("row sample: "); Serial.println(val_sens);

        //if (val_sens < 1600) {
        //    val_sens = 0;
        //}
        //else {
        //    val_sens = val_sens - 1600;
        //}
        ////Serial.print("sample: "); Serial.println(val_sens);
        //    
        //photoValue = (255 / 2495.0) * val_sens;
        //brigthsamples[sample_metter++] = 255 - photoValue;

        /*Serial.print("result: "); Serial.println(brigthsamples[sample_metter - 1]);
        Serial.println("-------------------------");*/
        photoValue = (255 / 4095.0) * analogRead(photoPin);
        brigthsamples[sample_metter++] = 255 - photoValue;

    }
    else {
        int sum_smpl = 0;
        for (size_t i = 0; i < PHOTO_SAMPLES; i++)
        {
            sum_smpl += brigthsamples[i];
        }
        brightness = sum_smpl / PHOTO_SAMPLES;
        sample_metter = 0;
        if (!brightness) { brightness = 10; }
        myLED.displayBrightness(brightness);

        Serial.println();
        Serial.print("New average brightness: ");
        Serial.println(brightness);

        /*for (size_t i = 0; i < sizeof(brigthsamples); i++)
        {
            Serial.print(brigthsamples[i]); Serial.print(" ");
        }
        Serial.println();*/
    }
}