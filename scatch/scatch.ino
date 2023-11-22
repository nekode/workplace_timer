#include <EEPROM.h>
#include <RotaryEncoder.h>
#include "TM1637.h"
#define CLK 3//pins definitions for the module and can be changed to other ports       
#define DIO 2
// #define LED_BUILTIN 13
#define key_pressed 1
#define key_holded 4
#define keys_not_pressed 0
TM1637 disp(CLK,DIO);

//библиотека tm1637.h изначально имеет баг, который надо править ручками.
//Открываете файл tm1637.cpp и добавляете в строку 93
//Команду break;
//  digitalWrite(Clkpin,LOW); //wait for the ACK
//  digitalWrite(Datapin,HIGH);
//  digitalWrite(Clkpin,HIGH);     
//  pinMode(Datapin,INPUT);
//  while(digitalRead(Datapin))
// { 
//    count1 +=1;
//    if(count1 == 200)//
//    {
//     pinMode(Datapin,OUTPUT);
//     digitalWrite(Datapin,LOW);
//     count1 =0;
//	 break; // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
//    }
//    pinMode(Datapin,INPUT);
//  }
//  pinMode(Datapin,OUTPUT);
//}
//send start signal to TM1637



// Setup a RoraryEncoder for pins A2 and A3:
RotaryEncoder encoder(A2, A3);

// константы
int preset_time = 5400; // заданное время в секундах

// переменные
int8_t TimeDisp[] = {0x00,0x00,0x00,0x00};
int lastPos = 0;
int halfseconds = 118;
const byte pushButton = 7;
const byte PIR = 5;
const byte buzzer = 4;
const byte relay = 6;
unsigned long prMillis=0;
uint8_t key_data = 0;

void setup()
{
  disp.set(BRIGHT_TYPICAL);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  disp.init(D4036B);//D4056A is the type of the module
//  disp.init(D4056A);//D4056A is the type of the module
  pinMode(pushButton, INPUT_PULLUP);
  pinMode(PIR, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);  
  pinMode (LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);  
  digitalWrite(buzzer, LOW);
  digitalWrite(relay, LOW);
  {
  int temp_preset_time = 15;
  EEPROM.get( 0, temp_preset_time);
  if ((temp_preset_time <= 11880) && (temp_preset_time >= 5)) 
	{
	halfseconds = temp_preset_time;
	preset_time = temp_preset_time;
	}
  else
	{
	halfseconds = preset_time;
	}
  }
} // setup()


// Read the current position of the encoder and print out when changed.
void loop()
{
  key_data = get_key();  // вызываем функцию определения нажатия кнопок, присваивая возвращаемое ней значение переменной, которую далее будем использовать в коде
  if (halfseconds <= 0)
  {
  digitalWrite(relay, HIGH);
  }
  if (digitalRead(PIR))
  {
  digitalWrite(LED_BUILTIN, HIGH);
//  digitalWrite(buzzer, HIGH);
  halfseconds = preset_time;
  }  
  else
  {
  digitalWrite(LED_BUILTIN, LOW); 
//  digitalWrite(buzzer, LOW); 
  }
  
  
//  byte ind = halfseconds/120;
//  if (halfseconds > 120)
//  {
//  disp.display(ind);
//  }
//  else
//  {
//  byte temp_ind = halfseconds/2;
//  disp.display(temp_ind);
//  }

  TimeDisp[3] =  (halfseconds % 20) / 2;
  TimeDisp[2] = (halfseconds % 120) / 20;
  TimeDisp[1] = (halfseconds % 1200) / 120;
  TimeDisp[0] = halfseconds / 1200;
  disp.display(TimeDisp);

  
  encoder.tick();

  int newPos = encoder.getPosition();  // get the current physical position

  if (((halfseconds < 240) && (halfseconds > 238)) || ((halfseconds < 120) && (halfseconds > 118)) || ((halfseconds < 360) && (halfseconds > 358)) || (halfseconds == 18) || (halfseconds == 16) || (halfseconds == 14) || (halfseconds == 12) || (halfseconds == 10) || (halfseconds == 8) || ((halfseconds <= 6) && (halfseconds > 0)))
  {
  digitalWrite(buzzer, HIGH);
  }
  else
  {
  if (digitalRead (buzzer)) {digitalWrite(buzzer, LOW);}
  }
  
  if (newPos < lastPos) 
  {
    halfseconds = halfseconds - 120;
  } 
  else if (newPos > lastPos) 
  {
   halfseconds = halfseconds + 120;
  } 
  if (lastPos != newPos) 
  {
    lastPos = newPos;
	preset_time = halfseconds;
  if (preset_time > 11880)
  {
  preset_time = 11880;
  }
  else if (preset_time < 7)
  {
  preset_time = 7;
  }
  }
  if (key_data == key_holded)
  {
  int temp_preset_time = (preset_time - (preset_time % 120));
  if (temp_preset_time <= 0) {temp_preset_time = 120;}
  EEPROM.put(0 , temp_preset_time);
  digitalWrite(buzzer, HIGH);
  delay(300);
  digitalWrite(buzzer, LOW);
  }
  if (key_data == key_pressed)
  {
  halfseconds = 7;
  }
  if (millis()-prMillis>=499) 
  {
  prMillis=millis();
  halfseconds --;
  if(halfseconds % 2)
  {
  disp.point(true);
  }
  else 
  {
  disp.point(false);
  } 
  }
  if (halfseconds > 11880)
  {
  halfseconds = 11880;
  }
  else if (halfseconds < 0)
  {
  halfseconds = 0;
  }
} 





byte get_key() // Функция определения нажатия и удержания кнопок
{
// версия 1 - для кратковременного нажатия значение возвращается при отпускании кнопки, для длительного - пока кнопка остаётся нажатой, с заданным интервалом
uint8_t trigger_push_hold_counter = 10; // задержка триггера кратковременного/длительного нажатия (проходов функции, умноженных на задержку "milliseconds_between_increment")  
uint8_t milliseconds_between_increment = 50; // интервал в миллисекундах между инкрементом счётчика нажатой кнопки 
static uint8_t val_kl;
static uint32_t key_delay_millis;
static uint32_t key_delay_after_hold_millis;
if ((millis() - key_delay_millis) > milliseconds_between_increment) //обрабатываем нажатия инкрементом переменной только если после предыдущей обработки прошло не менее "milliseconds_between_increment" миллисекунд
	{
	  if (!(PIND & (1 << PIND7)))  //нажатие
		{
		val_kl++;  // инкрементируем счётчик
		if (val_kl > trigger_push_hold_counter) // если значение счётчика больше порога детектирования удержания клавиши
			{
				val_kl = 0; // сбрасываем счётчик
				key_delay_after_hold_millis = millis(); // запоминаем время 
				return key_holded; // возвращаем значение
			}   
		}
	  key_delay_millis = millis(); // запоминаем время 
	}
if (val_kl > 0) //если клавиша перед этим была нажата 
	{
		if ((PIND & (1 << PIND7)) && ((millis() - key_delay_after_hold_millis) > (trigger_push_hold_counter * milliseconds_between_increment))) // если клавиша на данный момент отпущена и с момента последнего удержания любой клавиши прошёл интервал больше, чем один интервал удержания клавиши
			{
				val_kl = 0;  // сбрасываем счётчик  
				return key_pressed; // возвращаем значение
			}
	}
if (PIND & (1 << PIND7)) {val_kl = 0;} // если добрались до этой точки и кнопка не нажата - обнуляем счётчик (защита от появления "pressed" после "holded")
return 0; // если ни одна из кнопок не была нажата - возвращаем 0
}

// The End
