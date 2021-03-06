 /**
  * Этот файл является частью библиотеки MBee-Arduino.
  * 
  * MBee-Arduino является бесплатным программным обеспечением. 
  * Подробная информация о лицензиях находится в файле mbee.h.
  * 
  * \author </i> von Boduen. Special thanx to Andrew Rapp.
  */
      
#include <MBee.h>
#include <SoftwareSerial.h>

/** Скетч предназначен для демонстрации работы с библиотекой с помощью callback функций. 
  * Такой способ организации скетча позволяет избавиться от необходимости "вручную"
  * в функции loop() заниматься проверкой наличия поступивших от радиомодуля данных. Также 
  * автоматизируются многие действия, связанные с передачей пакетов.
  * Скетч предназначен для демонстрации приема и обработки пакета с данными о состоянии 
  * цифровых и аналоговых датчиков на удаленном модеме. Принимающий и передающий модули MBee-868-x.0 
  * работают под управлением ПО Serial Star.
  * Действия, производимые скетчем подробно описаны в комментариях к соответствующим строкам.
  * Потребуются 2 модуля MBee-868-x.0. Первый модуль соедининяется с платой Arduino c помощью 
  * XBee-shield или любого другого совместимого устройств. Если доступного шилда нет, то возможно 
  * соединение Arduino и модуля с помощью проводов.
  * ВНИМАНИЕ!!! Модуль MBee-868-x.0 имеет номинальное значение напряжения питания 3,3В. Если Ваша
  * плата Arduino имеет выходы с логическими уровнями 5В, то необходимо предусмотреть делитель 
  * напряжения между выходом TX Arduino и входом RX модуля (вывод №3 для всех моделей). К выводу TX
  * Arduino подключается резистор 2К, с которым соединен резистор 1К, второй вывод последнего
  * сажается на землю. Точка соединения резисторов соединяется с выводом №3 модуля. 
  * Вывод №2 модуля (TX), подключается ко входу RX Arduino через последовательный резистор 1К.
  * При использовании для питания модуля собственного источника 3,3В Arduino, необходимо помнить о том,
  * что модули могут потреблять в режиме передачи токи до 200 мА. Поэтому необходимо уточнять 
  * нагрузочные характеристики применяемой Вами платы Arduino. При коротких эфирных пакетах для 
  * компенсации недостаточного выходного тока источника 3,3В можно применить конденсаторы с емкостью
  * не менее 2200 мкФ, устанавливаемые параллельно питанию модуля.
  * На обоих модулях, после загрузки программного обеспечения SerialStar, должен быть произведен возврат 
  * к заводским настройкам одним из двух способов:
  * 1. Быстрое 4-х кратное нажатие "SYSTEM BUTTON" (замыкание вывода №36 модуля на землю).
  * 2. С помощью командного режима: +++, AT RE<CR>, AT CN<CR>.
  * 
  * Первый модуль должен быть предварительно настроен для работы в пакетном режиме с escape-
  * символами AP=2. Режим аппаратного управления потоком (CTS/RTS) должен быть отключен.
  * Последовательность для настройки: 
  * 1. +++
  * 2. AT AP2<CR>
  * 3. AT CN<CR>
  * 
  * Второй модуль должен быть установлен на плату MB-Tag, или любую другую, обеспечивающую 
  * питание 3,3В. На модуле необходимо включить режим периодического сна с помощью следующей  
  * последовательности команд:
  * 1. +++
  * 2. AT SM4<CR>
  * 3. AT CN<CR>
  * Все прочие настройки можно оставить "по умолчанию".
  * 
  * Диагностические сообщения и результаты выводятся на консоль с помощью SoftwareSerial.
  * Для проверки работоспособности, рекомендуется подключить в выводам ssRX и ssTX 
  * преобразователь USB-UART, например MB-USBridge, и воспользоваться любой удобной
  * терминальной программой.
  * Подключение Arduino к плате MB-USBridge:
  * Arduino    MB-USBridge    
  * GND     -->  X3.1
  * ssRX    -->  X3.8 
  * ssTX    -->  X3.9 
  */

//Назначаем выводы TX/RX для SoftSerial.
uint8_t ssRX = 8;
uint8_t ssTX = 9;

SoftwareSerial nss(ssRX, ssTX);
SerialStarWithCallbacks mbee = SerialStarWithCallbacks();

#define REQUESTED_PIN 34 //Номер вывода, используемый в примере запроса наличия в принятом пакете данных о его текущем состонии.

void setup() 
{ 
  Serial.begin(9600);
  mbee.begin(Serial);
  nss.begin(9600);
  //Регистрируем типы пакетов, которые требуется принимать.
  mbee.onRxIoSampleResponse(rxIoSampleResponseCallback); //Регистрируем функцию, которая будет автоматически вызываться каждый раз после приема пакета с данными о состоянии цифровых и аналоговых датчиков удаленного узла.
  delay(500); //Задержка не обязательна и вставлена для удобства работы с терминальной программой.
}

void loop() 
{
  mbee.run(); //Основной цикл, выполняющий все функции по приему API-фреймов от модуля.
}

//Зарегистрированная функция обработки пакета с данными о состоянии цифровых и аналоговых датчиков на удаленном модеме.
void rxIoSampleResponseCallback(RxIoSampleResponse& ioSample, uintptr_t optionalParameter)
{
  nss.println("");
  nss.print("Received packet with I/O samples from remote modem with address: ");
  nss.println(ioSample.getRemoteAddress(), HEX); //Печатаем адрес отправителя.
  nss.println("");

  /**********************************************************************************************************/
  nss.print("Temperature of the module is "); //Выводим температуру удаленного модема по показаниям встроенного датчика.
  if(ioSample.getTemperature() < 128) //Переводим число из дополнительного кода в прямой.
    nss.print(ioSample.getTemperature(), DEC);
  else
  {
    nss.print("-"); //Температура отрицательная.
    nss.print(256 - ioSample.getTemperature(), DEC);
  }
  nss.println("C.");
  
  /**********************************************************************************************************/
  nss.print("Supply voltage is "); //Печатаем измеренное напряжение источника питания удаленого модема.
  nss.print(float(ioSample.getVbatt()) / 51);
  nss.println("V.");
  
  /**********************************************************************************************************/
  if(ioSample.getSampleSize()) //Содержится ли в принятом пакете информация о состоянии линий ввода/вывода удаленного модема?
  {
    nss.print("There are(is) ");
    nss.print(ioSample.getSampleSize(), DEC);
    nss.println(" I/0 sample(s) in the packet.");
  }
  
  /**********************************************************************************************************/
  if(ioSample.isAvailable(REQUESTED_PIN)) //Пример запроса о наличии в принятом пакете данных о конкретном выводе модуля.
  {
    nss.print("Pin #");
    nss.print(REQUESTED_PIN);
    nss.println(" is sampled in the received packet.");
  }
  else
  {
    nss.print("Pin #");
    nss.print(REQUESTED_PIN);
    nss.println(" is NOT sampled in the received packet.");
  }
    
  /**********************************************************************************************************/  
  for(uint8_t i = 1; i <= ioSample.getSampleSize(); i++)// Выводим данные о текущем состоянии каждой линии ввода/вывода содержащейся а пакете.
  {
    nss.print("Pin #"); 
    nss.print(ioSample.getPin(i), DEC);
    nss.print(" is "); //Выводим идентификатор режима, на работу в котором настроена данная линия ввода/вывода.
    if((ioSample.getMode(i) & 0x7F) == IO_ADC) //Информация о текущем состоянии цифрового входа/выхода передается в старшем бите кода режима. Поэтому сбрасываем его, чтобы получить код режима.
    {
      nss.print("analog input. Voltage: ");
      nss.print(float(ioSample.getAnalog(i))* 2.5 /4096);
      nss.println("V.");
    }
    else if((ioSample.getMode(i) & 0x7F) == IO_DIGITAL_INPUT) 
    {
      nss.print("digital input. State: ");
      printDigitalPinState(ioSample.getDigital(i)); //Пример получения информации о текущем состоянии цифровой линии с помощью специальной функции.
     }
    else if((ioSample.getMode(i) & 0x7F) == IO_DIGITAL_OUTPUT_LO) 
    {
      nss.print("digital output with default LOW state. State: ");
      printDigitalPinState(ioSample.getMode(i) & 0x80); //Пример получения информации о текущем состоянии цифровой линии с помощью старшего бита кода режима. Такой способ работает намного быстрее, чем вызов функции getDigital().
    }
    else if((ioSample.getMode(i) & 0x7F) == IO_DIGITAL_OUTPUT_HI)
    {
      nss.print("digital output with default HIGH state. State: ");
      printDigitalPinState(ioSample.getMode(i) & 0x80);
    }
    
    else if((ioSample.getMode(i) & 0x7F) == IO_COUNTER_INPUT1)
    {
      nss.print("count input 1 with pullup. Pulse count: ");
      nss.print(ioSample.getCounter(i));
      nss.println(".");
    }
    else if((ioSample.getMode(i) & 0x7F) == IO_COUNTER_INPUT2)
    {
      nss.print("count input 2. Pulse count: ");
      nss.print(ioSample.getCounter(i));
      nss.println(".");
    }
    else if((ioSample.getMode(i) & 0x7F) == IO_WAKEUP_INPUT_FALLING_EDGE)
    {
      nss.print("wakeup(alarm) input with HIGH to LOW active front. State: ");
      printDigitalPinState(ioSample.getMode(i) & 0x80);
    }
    else if((ioSample.getMode(i) & 0x7F) == IO_WAKEUP_INPUT_RISING_EDGE)
    {
      nss.print("wakeup(alarm) input with LOW to HIGH active front. State: ");
      printDigitalPinState(ioSample.getMode(i) & 0x80);
    }
  }
}

void printDigitalPinState(uint8_t state)
{
  if(state)
    nss.println("HIGH.");
  else
    nss.println("LOW.");
}
        
