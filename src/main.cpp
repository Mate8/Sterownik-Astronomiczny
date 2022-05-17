#include <Arduino.h>
#include <DS3231.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <LiquidCrystal_I2C.h>
#include <SolarCalculator.h>
#include <TimeLib.h>
#include <Wire.h>

#define I2CADDR 0x20

LiquidCrystal_I2C lcd(0x3F, 16, 2);
DS3231 clock;
RTCDateTime dt;

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {{'1', '2', '3', 'A'},
                         {'4', '5', '6', 'B'},
                         {'7', '8', '9', 'C'},
                         {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {3, 2, 1, 0};
byte colPins[COLS] = {4, 5, 6, 7};

Keypad_I2C kpd(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR,
               PCF8574);

//----dotyczny wyswietlania customowych znakow na lcd----
#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args) write(args);
#else
#define printByte(args) print(args, BYTE);
#endif
//--------------------------------------------------------

#define xpot A0    // czujniki obrotu trackera (potencjometry)
#define ypot A1    //
#define N_motor 11 //  silnik połnoc    N   👆
#define S_motor 10 //  silnik połódnie  S   👇
#define W_motor 9  //   silnik zachód    E   👈
#define E_motor 8  //   silnik wschód    W   👉

char key;  // keypapd char
int state; // enum{ IDLE, PRESSED, HOLD, RELEASED }
int menu;  // pozycja w menu
bool flag1;
bool flag2;
bool flag3;
bool flag4;

bool stop_X; // zmienna pomocnicza
bool stop_Y; //
int Xangle;  // kąt w stopniach potencjometra obrotu
int Yangle;  //

double sunrise; // Sunrise, in hours (UTC)
double transit; // Solar noon, in hours (UTC)
double sunset;  // Sunset, in hours (UTC)
int azimuth_int, elevation_int;
double azimuth, elevation;

// imeges -> delta ONOFF (tam jest eyunek poglądowy kory moze pomoc lepiej zrozumiec ponizsze dwie zmienne)
//  Gdy tracker jest wewnątrz przedziału OFF to silniki sie wyłączają
//  gdy tracer  jest poza przedziałem ON to silniki próboją ustawić tracker w zadanej pozycji
//  delta ON jest takie duże ponieważ silniki sie szybko kręcą i w łatwy sposób mozna doprowadzic do tego ze
// sterownik bedzie naprzemiennie załączał silnik prawo lewo.
// normalnie w trackerach jeden obrót w osi X potrafi trwać po kilka minut tutaj jeden obrót to czas moze 3 sekund
const int delta_ON = 50;
const int delta_OFF = 5;
//--------------------------------------------------------------------------

//------OBRÓCENIE WARTOSCI NA POTENCJOMETRACH----funkcja reverse_pot_value--
const bool invert_X = true;
const bool invert_Y = true;
//--------------------------------------------------------------------------

//-------PRZERWANIE || wiatromierz--------------
#define kontaktron 7
volatile int impulsy;
int RPM;
unsigned long t1;
bool strong_wind = false;
unsigned strong_wind_time;
//-------------------------------------------------

//--------------------------- KOORDYNATY-----------------------------------------------------
const double latitude = 54;  // szerokosc geograficzna
const double longitude = 18; // dlugosc geograficzna
const int time_zone = +2;    // (+1) czas zimowy (+2) czas letni
const int interval = 1;      // interwał odswiezania w sekundach (ekranu)
//-------------------------------------------------------------------------------------------

//---POZYCJA PARKOWANIA TRACKERA PO ZACHODZIE LUB PRZYY ZBYT DUZYM WIETRZE-------------------
const int park_X = 180;       // 180 stopni (połódnie)
const int park_Y = 5;         // 0 stopni (leżąco) (wpisane 5 dla bezpieczenstwa poniewaz potencjometry nie zawsze musi wskazac 0 wynika to z rozdzielczosci komparatora i niedokładnosci potencjimetra  )
const int min_Elevation = 10; // jezeli sloncejest powyzej tego kątu to śledzimy
//------------------------------------------------------------------------------------------

//-------kąty obrotów potencjometru(300 stopni) ---- Dla własciwego zmapowania potencjometrów i odczytów kątów pokrywających sie z rzeczywistością.--
const int Xangle_min = 30; // dla osi X połodnie jest na 180 stopni więc ustalam minimalny kat i maksymalny kąt optymalnie
const int Xangle_max = 330;
const int Yangle_min = 0; // dla osi Y łatwiej jest skalować potencjometr od zera
const int Yangle_max = 300;
const int delta = 1; // gdy potencjometr jest np na maxa w prawo to zdarza sie że nie jest na wyjsciu 5V tylko 4.98 i po zmapowaniu nie da rade uzyskac idealnie krancowych wartosci
//--------------------------------------------

//------maksymalne wartosci wychyłów jakie tracker może osiągać----
const int tracker_Xmin = 0;
const int tracker_Xmax = 360;
const int tracker_Ymin = 0;  // płasko
const int tracker_Ymax = 90; // max podniesiony
//------------------------------------------------------------------
//-------Obliczone wartosci max i min wychyłow dla trackera-----
int T_XMAX;
int T_XMIN;
int T_YMAX;
int T_YMIN;
//--------------------------------------------------------------

void menu_item();
void menu_look();
void menu_execute();
void pozycjaslonca();
void godzina();
void manualny();
void manualnyBIS();
void AUTO();
void action1();
void action2();
void action3();
void action4();
void action5();
void zachwsch();
void printSunTime24h(double);
void move(int, int);
void interrupt_rpm();
void attachInterrupt();
void tracker_move_calculate();
int rpm();
int get_data(int);
void get_pot_value();
int perpendicular_Ytracker_degree(int);
// ^^^^ obliczenie kątu pochyłu ( kąt słonca i kąt płaszczyzny paneli PV sa inne, więc zmieniamy kąt dla Y zeby się zgadzało )
// ^^^^ np gdy słonce jest na lini horyzontu czyli ma kąt ok 0 stopni to tracker powinien być podniesiony jak najwyzej (idealnie by było 90 stopni)

#include "chars.h"
#include "motors.h"

void setup()
{
  Serial.begin(9600);
  lcd.init();

  Wire.begin();
  kpd.begin();
  kpd.setDebounceTime(10);
  lcd.backlight();
  lcd.setCursor(2, 0);
  lcd.print("SOLAR TRACKER");
  lcd.setCursor(0, 1);
  lcd.print("MATEUSZ ANIOLKOWSKI");

  lcd.createChar(0, uparrow_icon);
  lcd.createChar(1, downarrow_icon);
  lcd.createChar(2, azymut_icon);
  lcd.createChar(3, elewacja_icon);
  lcd.createChar(4, OK_icon);

  pinMode(xpot, INPUT);
  pinMode(ypot, INPUT);
  pinMode(N_motor, OUTPUT);
  pinMode(S_motor, OUTPUT);
  pinMode(W_motor, OUTPUT);
  pinMode(E_motor, OUTPUT);

  pinMode(kontaktron, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(kontaktron), interrupt_rpm, FALLING);

  clock.begin();
  delay(300);
  lcd.clear();

  tracker_move_calculate();
}
void loop()
{

  static unsigned long next_millis = 0;
  if (millis() > next_millis)
  {

    lcd.setCursor(0, 1);
    godzina();
    lcd.setCursor(2, 0);
    lcd.print("WITAJ W MENU");
    lcd.setCursor(9, 1);
    lcd.print(clock.readTemperature());
    lcd.setCursor(8, 1);
    lcd.print((char)255);
    lcd.setCursor(14, 1);
    lcd.print((char)223);
    lcd.print("C");
    next_millis = millis() + interval * 1000;
  }

  key = ' '; // po to żeby nie wchodziło przypadkowo do case D po wyjsciu z ustawień

  char key = kpd.getKey();

  switch (key)
  {
  case 'A':
    manualny();
    break;

  case 'B':
    manualnyBIS();
    break;

  case 'C':
    AUTO();
    break;

  case 'D':
    menu_look();
    break;
  }
}

void get_pot_value()
{
  if (invert_X == false)
  {
    Xangle = map(analogRead(xpot), 0, 1023, Xangle_min, Xangle_max);
  }

  if (invert_Y == false)
  {
    Yangle = map(analogRead(ypot), 0, 1023, Yangle_min, Yangle_max);
  }

  if (invert_X == true)
  {
    Xangle = map(analogRead(xpot), 0, 1023, Xangle_max, Xangle_min);
  }

  if (invert_Y == true)
  {
    Yangle = map(analogRead(ypot), 0, 1023, Yangle_max, Yangle_min);
  }
}

int rpm()
{
  if (millis() - t1 > 1000)
  {
    RPM = impulsy * 60;
    impulsy = 0;
    t1 = millis();
  }
  return RPM;
}

void interrupt_rpm()
// zliczanie impulsów z uwzględnieniem czasu drganiastyków na kontaktronie
{
  static unsigned long lastTime;
  unsigned long timeNow = millis();
  if (timeNow - lastTime < 50)
    return;
  impulsy++;
  lastTime = timeNow;
  Serial.println(impulsy);
}

void AUTO()
{
  unsigned int time1, time2;

  lcd.clear();

  while (true)
  {
    key = kpd.getKey();

    pozycjaslonca(); // odwołanie do prostej funkcji w której biblioteka
                     // wykonuje obliczenia aby okreslic aktualna pozycje
                     // slonca

    get_pot_value();

    lcd.setCursor(0, 1);
    lcd.printByte(2);
    lcd.print(azimuth);
    lcd.print((char)223);
    lcd.setCursor(9, 1);
    lcd.printByte(3);
    lcd.print(elevation);
    lcd.print((char)223);

    lcd.setCursor(0, 0);
    lcd.print("X:");
    lcd.print(Xangle);
    lcd.print("  ");
    lcd.setCursor(6, 0);
    lcd.print("Y:");
    lcd.print(Yangle);
    lcd.print("  ");
    lcd.setCursor(12, 0);
    lcd.print(rpm());
    lcd.print("     ");

    time1 = millis() / 1000; // odmierzanie co sekunde
    if (time1 != time2)
    {
      time2 = time1;

      if (rpm() > 100) // jezeli wiatromierz odczytuje większe wartosci od 100 to naliczamy ile trwa ten silny wiatr
      {
        strong_wind_time++;
        if (strong_wind_time > 5)
        {
          strong_wind = true;
        }
      }
      else // jesli wiatr osłabł to odczekujemy chwile to pozwalamy trackerowi sie podniesc.
      {
        if (strong_wind_time == 0)
        {
          strong_wind = false;
        }
        else if (strong_wind_time > 5)
        {
          strong_wind_time = 5;
        }
        else
        {
          strong_wind_time--;
          strong_wind = true;
        }
      }
    }

    if (elevation_int <= min_Elevation || strong_wind == true) // jesli slonce jest ponizej, lub blisko horyzontu i
                                                               // silny wiatr trwa wiecej niz 5 sekund to kładziemy tracker na płasko.
    {
      move(park_X, park_Y);
      lcd.setCursor(8, 1);

      if ((millis() / 1000) % 2 == 0)
      {                 // wykrywanie parzystej sekundy
        lcd.print("P"); // oznacza pozycje "parking"
      }
      else
      {
        lcd.print(" ");
      }
    }
    else
    {
      move(azimuth_int, elevation_int);
      lcd.setCursor(8, 1);
      lcd.printByte(4);
    }

    if (key == '*')
    {
      lcd.clear();
      digitalWrite(N_motor, LOW);
      digitalWrite(S_motor, LOW);
      digitalWrite(W_motor, LOW);
      digitalWrite(E_motor, LOW);
      break;
    }
  }
}

void menu_item()
{
  while (true)
  {
    key = kpd.getKey();
    state = kpd.getState();

    if (state == 1 && key == '8')
    {
      menu++;
      menu_look();
    }
    if (state == 1 && key == '2')
    {
      menu--;
      menu_look();
    }
    if (state == 1 && key == '5')
    {
      menu_execute();
      menu_look();
      while (key == '5')
        ;
    }

    if (menu < 1)
    {
      menu = 1;
    }

    if (key == '*')
    {
      lcd.clear();
      break;
    }
    Serial.println(menu);
  }
}

void menu_look()
{
  switch (menu)
  {
  case 0:
    menu = 1;
    break;
  case 1:
    lcd.clear();
    lcd.print(">Pozycja slonca");
    lcd.setCursor(0, 1);
    lcd.print(" godz-wsch/zach");
    menu_item();
    break;
  case 2:
    lcd.clear();
    lcd.print(" Pozycja slonca");
    lcd.setCursor(0, 1);
    lcd.print(">godz-wsch/zach");
    menu_item();
    break;
  case 3:
    lcd.clear();
    lcd.print(">ustawianie godziny");
    lcd.setCursor(0, 1);
    lcd.print("----------------");
    menu_item();
    break;
  case 4:
    menu = 3;
    break;
  }
}

void menu_execute()
{
  switch (menu)
  {
  case 1:
    action1(); // Pozycja slonca
    break;
  case 2:
    action2(); // godz-wsch/zach
    break;
  case 3:
    action3(); // ustawianie godziny
    break;
  }
}

void action1() // Pozycja slonca
{
  lcd.clear();
  key = kpd.getKey();

  while (true)
  {
    key = kpd.getKey();
    pozycjaslonca();

    static unsigned long next_millis = 0;
    if (millis() > next_millis)
    {
      lcd.setCursor(0, 0);
      lcd.print("AZ");
      lcd.setCursor(14, 0);
      lcd.print("EL");
      lcd.setCursor(0, 1);
      lcd.printByte(2);
      lcd.print(azimuth);
      lcd.print((char)223);
      lcd.setCursor(9, 1);
      lcd.printByte(3);
      lcd.print(elevation);
      lcd.print((char)223);
      lcd.setCursor(4, 0);
      godzina();
      next_millis = millis() + interval * 1000;
    }

    if (key == '*')
    {
      break;
    }
  }
}

void action2() // godz-wsch/zach
{
  zachwsch();
}

void action3() // ustawianie godziny
{
  lcd.clear();

  while (true)
  {
    key = kpd.getKey();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rok: ");
    int year = get_data(dt.year);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Miesiac: ");
    int month = get_data(dt.month);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dzien: ");
    int day = get_data(dt.day);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Godzina: ");
    int hour = get_data(dt.hour);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Minuty: ");
    int minute = get_data(dt.minute);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sekundy: ");
    int second = get_data(dt.second);

    clock.setDateTime(year, month, day, hour, minute, second);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ustawiono ");
    lcd.setCursor(0, 1);
    lcd.print("Date i Godzine ");
    delay(700);
    break;
  }
}

int get_data(int type_data)
{
  String container = "\0";
  lcd.setCursor(0, 1);
  lcd.print("            #>OK");
  lcd.setCursor(0, 1);
  lcd.print(type_data);
  while (true)
  {
    char key = kpd.getKey();
    if (key == '#' && container != "\0")
    {
      break;
    }
    else if (key == '#' &&
             container == "\0")
    { // jezeli nie wpiszemy zadnej wartosci
      // zostawiamy starą wartosc
      container = type_data;
      break;
    }
    else if (isDigit(key))
    {
      container += key;
      lcd.setCursor(0, 1);
      lcd.print("            #>OK");
      lcd.setCursor(0, 1);
      lcd.print(container);
    }
    else if (key == '*')
    {
      container = "\0";
      lcd.setCursor(0, 1);
      lcd.print("            #>OK");
    }
  }
  return container.toInt();
}

void pozycjaslonca()
{
  dt = clock.getDateTime();
  calcHorizontalCoordinates(dt.year, dt.month, dt.day, dt.hour - time_zone,
                            dt.minute, dt.second, latitude, longitude, azimuth,
                            elevation);
  azimuth_int = (int)azimuth;
  elevation_int = (int)elevation;
}

void godzina()
{
  dt = clock.getDateTime();
  clock.forceConversion();

  if (dt.hour < 10)
  {
    lcd.print("0");
    lcd.print(dt.hour);
    lcd.print(":");
  }
  else
  {
    lcd.print(dt.hour);
    lcd.print(":");
  }

  if (dt.minute < 10)
  {
    lcd.print("0");
    lcd.print(dt.minute);
    lcd.print(":");
  }
  else
  {
    lcd.print(dt.minute);
    lcd.print(":");
  }

  if (dt.second < 10)
  {
    lcd.print("0");
    lcd.print(dt.second);
  }
  else
  {
    lcd.print(dt.second);
  }
}

void zachwsch()
{
  lcd.clear();

  while (true)
  {
    calcSunriseSunset(dt.year, dt.month, dt.day, latitude, longitude, transit,
                      sunrise, sunset);
    lcd.setCursor(3, 0);
    lcd.print(dt.day);
    lcd.print(":");
    lcd.print(dt.month);
    lcd.print(":");
    lcd.print(dt.year);
    lcd.print("r");

    lcd.setCursor(0, 1);
    lcd.printByte(0);
    printSunTime24h(sunrise);
    lcd.setCursor(9, 1);
    lcd.printByte(1);
    printSunTime24h(sunset);

    key = kpd.getKey();

    if (key == 'B')
    {
      manualnyBIS();
    }
    if (key == 'A')
    {
      manualny();
    }
    if (key == '*')
    {
      break;
    }
  }
}

void printSunTime24h(double h)
{
  int m = int(round((h + time_zone) * 60));
  int hours = (m / 60) % 24;
  int minutes = m % 60;
  lcd.print(hours);
  lcd.print(":");
  lcd.print(minutes);
}
