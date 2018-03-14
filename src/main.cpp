#include <Adafruit_NeoPixel.h>
#include <Adafruit_DotStar.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
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
#define PIXEL_PIN 1
#define PIXEL_TYPE NEO_RGBW + NEO_KHZ800 //NEO_GRBW + NEO_KHZ800 

#define candleOffTime 10000
#define threshold 1.0

Candle candle = Candle(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE, EVERY_PIXEL);

//#define DATAPIN    7
//#define CLOCKPIN   8
//Adafruit_DotStar pixel = Adafruit_DotStar(NUMTRINKETLED, DATAPIN, CLOCKPIN, DOTSTAR_BRG);

//Accelerometer
/* Assign a unique ID to this sensor at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);

RunningAverage X(5);
RunningAverage Y(5);
RunningAverage Z(5);

void checkMovement();
void blowOffCandle();

void setup(void)
{
  Serial.begin(115200);

  /* Initialise the sensor */
  if(!accel.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  accel.setRange(ADXL345_RANGE_4_G);
 
  /* Display some basic information on this sensor */
  displaySensorDetails();
  
  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();
  displayRange();
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
  candle.update();

  static uint32_t lastFlashMillis = 0;
  if(millis() - lastFlashMillis > 50)
  {
    checkMovement();
    lastFlashMillis = millis();
  }

  if(Z.getAverage() > threshold)
  {
    blowOffCandle();
    X.clear(); // explicitly start clean
    Y.clear(); // explicitly start clean
    Z.clear(); // explicitly start clean
    delay(candleOffTime);
  }else
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

void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" m/s^2");  
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void displayDataRate(void)
{
  Serial.print  ("Data Rate:    "); 
  
  switch(accel.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print  ("3200 "); 
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print  ("1600 "); 
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print  ("800 "); 
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print  ("400 "); 
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print  ("200 "); 
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print  ("100 "); 
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print  ("50 "); 
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print  ("25 "); 
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print  ("12.5 "); 
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print  ("6.25 "); 
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print  ("3.13 "); 
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print  ("1.56 "); 
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print  ("0.78 "); 
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print  ("0.39 "); 
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print  ("0.20 "); 
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print  ("0.10 "); 
      break;
    default:
      Serial.print  ("???? "); 
      break;
  }  
  Serial.println(" Hz");  
}

void displayRange(void)
{
  Serial.print  ("Range:         +/- "); 
  
  switch(accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print  ("16 "); 
      break;
    case ADXL345_RANGE_8_G:
      Serial.print  ("8 "); 
      break;
    case ADXL345_RANGE_4_G:
      Serial.print  ("4 "); 
      break;
    case ADXL345_RANGE_2_G:
      Serial.print  ("2 "); 
      break;
    default:
      Serial.print  ("?? "); 
      break;
  }  
  Serial.println(" g");  
}