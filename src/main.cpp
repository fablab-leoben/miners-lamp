#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LIS3DH.h>
#include "RunningAverage.h"

void displayDataRate();
void displayRange();
void displaySensorDetails();

enum CandleStates{
  BURN_CANDLE,
  FLICKER_CANDLE,
  FLUTTER_CANDLE,
  MODES_MAX_CANDLE
};

enum PixelSelect{
  EVERY_PIXEL,
  SINGLE_PIXEL,
};

class Candle : public Adafruit_NeoPixel
{
  public:
    Candle(uint16_t count, uint8_t pin, uint8_t type);
    Candle(uint16_t count, uint8_t pin, uint8_t type, PixelSelect pixel, uint32_t pixNum = 0);
    ~Candle(){};
    void update();

  private:
    bool fire(uint8_t greenDropValue, uint32_t cycleTime);

    PixelSelect _pixelMode = EVERY_PIXEL;
    uint32_t _pixNum = 0;
    CandleStates _mode;
    uint32_t _lastModeChange;
    uint32_t _modeDuration;

    uint8_t _redPx = 255;
    uint8_t _bluePx = 15; //10 for 5v, 15 for 3.3v
    uint8_t _grnHigh = 135; //110-120 for 5v, 135 for 3.3v
    uint8_t _grnPx = 100;

    uint32_t _lastBurnUpdate = 0;
    int _direction = 1;
};

Candle::Candle(uint16_t count, uint8_t pin, uint8_t type) : Adafruit_NeoPixel(count, pin, type)
{
  randomSeed(micros());
  _mode = BURN_CANDLE;
}

Candle::Candle(uint16_t count, uint8_t pin, uint8_t type, PixelSelect pixel, uint32_t pixNum) : Adafruit_NeoPixel(count, pin, type)
{
  _pixelMode = pixel;
  _pixNum = pixNum;
}

void Candle::update()
{
  if(millis() - _lastModeChange > _modeDuration)
  {
    _mode = static_cast<CandleStates>(random(MODES_MAX_CANDLE));
    _modeDuration = random(1000, 8000);
    _lastModeChange = millis();
    //Serial.printlnf("New state: %d\tTime: %d", static_cast<int>(_mode), _modeDuration);
  }
  switch(_mode)
  {
    case BURN_CANDLE:
      this->fire(10, 120);
      break;
    case FLICKER_CANDLE:
      this->fire(15, 120);
      break;
    case FLUTTER_CANDLE:
      this->fire(30, 120);
      break;
  };
}

bool Candle::fire(uint8_t greenDropValue, uint32_t cycleTime)
{
  int currentMillis = millis();
  if(currentMillis - _lastBurnUpdate > (cycleTime / greenDropValue / 2))
  {
    _grnPx = constrain(_grnPx += _direction, _grnHigh - greenDropValue, _grnHigh);
    if(_grnPx == _grnHigh - greenDropValue or _grnPx == _grnHigh)
    {
      _direction *= -1;
    }
    switch (_pixelMode)
    {
      case EVERY_PIXEL:
        for(int i = 0; i < this->numPixels(); i++)
        {
          this->setPixelColor(i, _grnPx, _redPx, _bluePx);
        }
        break;
      case SINGLE_PIXEL:
        this->setPixelColor(_pixNum, _grnPx, _redPx, _bluePx);
        break;
    }
    this->show();
    _lastBurnUpdate = currentMillis;
  }
}

#define PIXEL_COUNT 7
#define PIXEL_PIN 3
#define PIXEL_TYPE NEO_RGBW + NEO_KHZ800 //NEO_GRBW + NEO_KHZ800 

#define threshold 1.0
#define candleOffTime 10000
uint32_t offTimeMillis = 0;
bool candleBurning = true;

Candle candle = Candle(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE, EVERY_PIXEL);

//#define DATAPIN    7
//#define CLOCKPIN   8
//Adafruit_DotStar pixel = Adafruit_DotStar(NUMTRINKETLED, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

//Accelerometer
/* Assign a unique ID to this sensor at the same time */
Adafruit_LIS3DH accel = Adafruit_LIS3DH();

RunningAverage X(5);
RunningAverage Y(5);
RunningAverage Z(5);

void checkMovement();
void blowOffCandle();

void setup(void)
{
  Serial.begin(115200);

  delay(10000);
  /* Initialise the sensor */
  if(!accel.begin(0x19))
  {
    /* There was a problem detecting the LIS3DH ... check your connections */
    Serial.println("Ooops, no LIS3DH detected ... Check your wiring!");
    while(1);
  }

  accel.setRange(LIS3DH_RANGE_4_G);   // 2, 4, 8 or 16 G!
 
  Serial.print("Range = "); Serial.print(2 << accel.getRange());  
  Serial.println("G");
  Serial.println("");

  X.clear(); // explicitly start clean
  Y.clear(); // explicitly start clean
  Z.clear(); // explicitly start clean

  candle.begin();
  candle.show();
  Serial.println("Started program");
}

void loop(void)
{
  if(candleBurning == true)
  {
    candle.update();
  } 

  static uint32_t lastFlashMillis = 0;
  if(millis() - lastFlashMillis > 50)
  {
    checkMovement();
    lastFlashMillis = millis();
  }

  if(Z.getAverage() > threshold)
  {
    candleBurning = false;
    blowOffCandle();
    X.clear(); // explicitly continue clean
    Y.clear(); // explicitly continue clean
    Z.clear(); // explicitly continue clean
    offTimeMillis = millis();
  } else if(candleBurning == false && millis() - offTimeMillis > candleOffTime)
    {
      candleBurning = true;
      offTimeMillis = 0;
    } else
      {
        //do nothing
      }
}

void checkMovement(void)
{
  /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);
 
  X.addValue(event.acceleration.x);
  Y.addValue(event.acceleration.y);
  Z.addValue(event.acceleration.z - 9.6);
  
  Serial.print(String(X.getAverage()));
  Serial.print("\t");
  Serial.print(String(Y.getAverage()));
  Serial.print("\t");
  Serial.println(String(Z.getAverage()));
}

void blowOffCandle(void)
{
  Serial.println("blowing off candle");
  for(int i = 0; i < candle.numPixels(); i++)
  {
    candle.setPixelColor(i, 0);
  }
  candle.show();
}