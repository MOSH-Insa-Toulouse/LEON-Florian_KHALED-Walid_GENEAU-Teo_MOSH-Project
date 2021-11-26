/*
 * Author: JP Meijers
 * Date: 2016-02-07
 * Previous filename: TTN-Mapper-TTNEnschede-V1
 *
 * This program is meant to be used with an Arduino UNO or NANO, conencted to an RNxx3 radio module.
 * It will most likely also work on other compatible Arduino or Arduino compatible boards, like The Things Uno, but might need some slight modifications.
 *
 * Transmit a one byte packet via TTN. This happens as fast as possible, while still keeping to
 * the 1% duty cycle rules enforced by the RN2483's built in LoRaWAN stack. Even though this is
 * allowed by the radio regulations of the 868MHz band, the fair use policy of TTN may prohibit this.
 *
 * CHECK THE RULES BEFORE USING THIS PROGRAM!
 *
 * CHANGE ADDRESS!
 * Change the device address, network (session) key, and app (session) key to the values
 * that are registered via the TTN dashboard.
 * The appropriate line is "myLora.initABP(XXX);" or "myLora.initOTAA(XXX);"
 * When using ABP, it is advised to enable "relax frame count".
 *
 * Connect the RN2xx3 as follows:
 * RN2xx3 -- Arduino
 * Uart TX -- 10
 * Uart RX -- 11
 * Reset -- 12
 * Vcc -- 3.3V
 * Gnd -- Gnd
 *
 * If you use an Arduino with a free hardware serial port, you can replace
 * the line "rn2xx3 myLora(mySerial);"
 * with     "rn2xx3 myLora(SerialX);"
 * where the parameter is the serial port the RN2xx3 is connected to.
 * Remember that the serial port should be initialised before calling initTTN().
 * For best performance the serial port should be set to 57600 baud, which is impossible with a software serial port.
 * If you use 57600 baud, you can remove the line "myLora.autobaud();".
 *
 */
#include <rn2xx3.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>

#define interruptPin 2

SoftwareSerial mySerial(10, 11); // RX, TX

//create an instance of the rn2xx3 library,
//giving the software serial as port to use
rn2xx3 myLora(mySerial);

// the setup routine runs once when you press reset:
void setup()
{
  pinMode(interruptPin, INPUT);
  //output LED pin
  pinMode(13, OUTPUT);
  led_on();

  // Open serial communications and wait for port to open:
  Serial.begin(57600); //serial port to computer
  mySerial.begin(9600); //serial port to radio
  Serial.println("Startup");

  initialize_radio();

  //transmit a startup message
  myLora.tx("TTN Mapper on TTN Enschede node");

  led_off();
  delay(2000);
}


void initialize_radio()
{
  //reset rn2483
  pinMode(12, OUTPUT);
  digitalWrite(12, LOW);
  delay(500);
  digitalWrite(12, HIGH);


  delay(100); //wait for the RN2xx3's startup message
  mySerial.flush();
  //Autobaud the rn2483 module to 9600. The default would otherwise be 57600.
  myLora.autobaud();
  String t = myLora.sendRawCommand("sys get ver");
  Serial.println(t);

  //check communication with radio
  String hweui = myLora.hweui();
  while(hweui.length() != 16)
  {
    Serial.println("Communication with RN2xx3 unsuccessful. Power cycle the board.");
    Serial.println(hweui);
    delay(10000);
    hweui = myLora.hweui();
  }

  //print out the HWEUI so that we can register it via ttnctl
  Serial.println("When using OTAA, register this DevEUI: ");
  Serial.println(myLora.hweui());
  Serial.println("RN2xx3 firmware version:");
  Serial.println(myLora.sysver());

  //configure your keys and join the network
  Serial.println("Trying to join TTN");
  bool join_result = false;


  /*
   * ABP: initABP(String addr, String AppSKey, String NwkSKey);
   * Paste the example code from the TTN console here:
   */
  const char *devAddr = "260BA7FB";
  const char *nwkSKey = "29B347E02560C50099A230376243253E";
  const char *appSKey = "9B4B24C6CE8A2C16ECA8EA7DB1E42960";

  join_result = myLora.initABP(devAddr, appSKey, nwkSKey);

  /*
   * OTAA: initOTAA(String AppEUI, String AppKey);
   * If you are using OTAA, paste the example code from the TTN console here:
   */
  //const char *appEui = "260B1AE1";
  //const char *appKey = "3D1351C97C367C1E063B5D99C24C266E";
  //join_result = myLora.initOTAA(appEui, appKey);


  while(!join_result)
  {
    Serial.println("Unable to join. Are your keys correct, and do you have TTN coverage?");
    delay(60000); //delay a minute before retry
    join_result = myLora.init();
  }
  Serial.println("Successfully joined TTN");

}

void read_data() {
    float sensor_volt;
    float sensorValue;
   
    sensorValue = analogRead(A0);
    sensor_volt = sensorValue/1024*5.0;
    //sensor_volt *= 100;
    Serial.print("sensor_volt = ");
    Serial.print(sensor_volt);
    Serial.println("V");

    uint32_t data = sensor_volt*100;
    byte payload[2];
    payload[0] = highByte(data);
    Serial.print("highByte = ");
    Serial.println(payload[0]);
    payload[1] = lowByte(data);
    Serial.print("lowByte = ");
    Serial.println(payload[1]);
    Serial.println("TXing");

    myLora.txBytes(payload, sizeof(payload)); //one byte, blocking function

    led_off();
}

void goingToSleep() {
  sleep_enable();
  Serial.println("dodo...");
  delay(100);
  attachInterrupt(0, wakeUp, RISING);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //delay(500);
  sleep_mode();
  Serial.println("Waking up...");
  
}

void wakeUp() {
  Serial.println("Interrupt Fired");
  sleep_disable();
  detachInterrupt(0);
}
// the loop routine runs over and over again forever:
void loop()
{
    goingToSleep();
    for (int i = 0; i < 3; i++) {
      read_data();
      Serial.print("Envoi n°");
      Serial.println(i);
      delay(200);
    }
}

void led_on()
{
  digitalWrite(13, 1);
}

void led_off()
{
  digitalWrite(13, 0);
}
