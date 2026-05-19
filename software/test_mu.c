// Программа для тестирования новых и штатных модулей памяти УЧПУ станка TDP.
// Версия 1.03 от 12.01.2025 (рабочая).
// Данные для записи в МП и считанные из МП передаются по интерфейсу RS-232 (COM-порт).
// Для создания COM-порта используется модуль USB-UART TTL CH340G.
// Внешнее устройство - базовая плата A-ESTF V2 ALTERA с ПЛИС CYCLONE IV E EP4CE10F17C8N.
// Базовая плата подключена к интерфейсной плате (с преобразователями уровней),
// к которой подключен тестируемый МП.
// Весь массив памяти тестируемого модуля состоит из 4-х страниц.
// Объём каждой страницы МП составляет 4096 слов. Разрядность слова - 17 бит.
// Для передачи (приёма) одного слова используются 3 байта.
// Размер файла данных (в байтах) определяется общим объёмом памяти МП
// и равен C = 4*4096*3 = 49152 (байт).
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <locale.h>
#include <malloc.h>

//void GoToXY(const int X, const int Y)
//{
//  HANDLE OutputHandle;
//  CONSOLE_SCREEN_BUFFER_INFO ScreenBufInfo;
//  OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
//  GetConsoleScreenBufferInfo(OutputHandle, &ScreenBufInfo);
//  ScreenBufInfo.dwCursorPosition.X = X;
//  ScreenBufInfo.dwCursorPosition.Y = Y;
//  SetConsoleCursorPosition(OutputHandle, ScreenBufInfo.dwCursorPosition);
//}

int main(int argc, char **argv)
{
HANDLE hCom; // Дескриптор COM-порта.
DCB dcb;
COMMTIMEOUTS ct;

unsigned char buf_out[1];
unsigned char buf_in[1];
DWORD oSize = sizeof(buf_out);
DWORD BytesWritten;
DWORD iSize;

FILE* f_in;
FILE* f_out;

// unsigned char RX_Code, TX_Code;
unsigned int i = 0;
int key;

  setlocale(LC_ALL, "");

  // Открыть COM-порт.
  if (argc < 2)
  {
    printf("Укажите имя COM-порта в параметре запуска программы в форме: test_mu.exe COMNAME\n");
    printf("Например, test_mu.exe COM3\n");
    getchar();
    return 1;
  }

  // Открыть COM-порт.
  hCom = CreateFile(argv[1], GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

  if (hCom == INVALID_HANDLE_VALUE)
  {
    printf("Ошибка! Не могу открыть COM-порт %s\n", argv[1]);
    printf("Проверьте подключение модуля конвертера USB-UART и его драйвер.\n");
    getchar();
    return 1;
  }
  else
  {
    // Получить текущие параметры COM-порта.
    if (!(GetCommState(hCom, &dcb)))
    {
      printf("Ошибка вызова GetCommState. Не удалось получить параметры COM-порта.\n");
      getchar();
      return 1;
    }

    // Вывести исходные параметры открытого COM-порта.
    printf("Параметры открытого COM-порта %s.\n", argv[1]);
    printf("fBinary = %d\n", (int)dcb.fBinary);
    printf("fParity = %d\n", (int)dcb.fParity);
    printf("BaudRate = %d\n", (int)dcb.BaudRate);
    printf("ByteSize = %d\n", (int)dcb.ByteSize);
    printf("Parity = %d\n", (int)dcb.Parity);
    printf("StopBits = %d\n", (int)dcb.StopBits);
    printf("fDtrControl = %d\n", (int)dcb.fDtrControl);
    printf("fRtsControl = %d\n", (int)dcb.fRtsControl);
    printf("fAbortOnError = %d\n", (int)dcb.fAbortOnError);
    printf("fOutxCtsFlow = %d\n", (int)dcb.fOutxCtsFlow);
    printf("fOutxDsrFlow = %d\n", (int)dcb.fOutxDsrFlow);
    printf("fNull = %d\n", (int)dcb.fNull);
    printf("fErrorChar = %d\n\n", (int)dcb.fErrorChar);

    // Инициализация структуры dcb для задания параметров COM-порта.
    ZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    // Настроить параметры COM-порта.
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.BaudRate = CBR_38400;  // Скорость приёма/передачи в Бодах.
    dcb.ByteSize = 8;          // Число битов данных.
    dcb.Parity = NOPARITY;     // Нет бита чётности.
    dcb.StopBits = ONESTOPBIT; // Один стоповый бит.
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = TRUE;
    dcb.fNull = TRUE;

    ct.ReadIntervalTimeout = 0xFFFFFFFF; // 10
    ct.ReadTotalTimeoutMultiplier = 0;   // 10
    ct.ReadTotalTimeoutConstant = 0;     // 10
    ct.WriteTotalTimeoutMultiplier = 0;
    ct.WriteTotalTimeoutConstant = 0;

    if (!(SetCommState(hCom, &dcb)))
    {
      printf("Ошибка вызова SetCommState. Не удалось установить параметры COM-порта.\n");
      getchar();
      return 1;
    }

    if (!(SetCommTimeouts(hCom, &ct)))
    {
      printf("Ошибка вызова SetCommTimeouts. Не удалось установить параметры COM-порта.\n");
      getchar();
      return 1;
    }

    // Прекратить все операции ввода/вывода через COM-порт.
    if (!(PurgeComm(hCom, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR)))
    {
      printf("Ошибка вызова PurgeComm.\n");
      getchar();
      return 1;
    }

    // Установить размер буферов ввода и вывода в байтах.
    if (!(SetupComm(hCom, 256, 256)))
    {
      printf("Ошибка вызова SetupComm.\n");
      getchar();
      return 1;
    }

    printf("COM-порт %s открыт успешно.\n", argv[1]);
    printf("Параметры COM-порта: скорость 38400 Бод, нет проверки на чётность, 8 бит данных, 1 стоп-бит.\n");
  }

Key_Input:

  printf("Режим работы:\n");
  printf("r - Чтение данных из COM-порта в файл.\n");
  printf("w - Запись данных из файла в COM-порт.\n");
  printf("t - Тестирование (запись данных из файла и чтение данных в файл).\n");
  printf("q - Выход.\n");
  key = getchar();
  if (key == 'r') goto Read_COM;
  if (key == 'w') goto Write_COM;
  if (key == 't') goto Write_COM;
  if (key == 'q') goto Quit_COM;
  goto Key_Input;

  // ***** Работа с COM-портом. *****

// Чтение данных из МП.
Read_COM:
  i = 0;

  // Открыть файл для записи считанных из COM-порта байтов.
  f_in = fopen("r_data.txt", "wb");

  printf("Для выхода нажмите любую клавишу.\n");

  // ***** Приём данных от ПЛИС. *****
  printf("Идёт приём данных от модуля памяти ...\n");

  while(!(kbhit())) // Выход при нажатии любой клавиши.
  {
    if (ReadFile(hCom, &buf_in, sizeof(buf_in), &iSize, 0))
    {
      if (iSize == 1)
      {
        // Принятый байт из COM-порта.
        printf("%02Xh ", buf_in[0]); // Вывод символов на экран ПК занимает время.

        // Записать принятый байт в файл r_data.txt.
        fwrite(buf_in, 1, 1, f_in);
        i = i + 1; // Увеличить счётчик считанных байтов из файла.
        //Sleep(1);  // Пауза 1 мс.
        if (i == 49152) // Завершить считывание данных из МП.
        {
          printf("\nЧтение данных из модуля памяти завершено.\n");
          break;
        }
      }
    }
    else
    {
      printf("Ошибка COM-порта при приёме байта.\n");
      getchar();
      return 1;
    }
  }
  fclose(f_in);
  goto Quit_COM;

// Запись данных в МП.
Write_COM:
  i = 0;

  // Открыть файл для чтения байтов данных, передаваемых в COM-порт.
  f_out = fopen("w_data_mix.txt", "rb");

  printf("Для выхода нажмите любую клавишу.\n");

  // ***** Передача данных в ПЛИС. *****
  printf("Идёт передача данных в модуль памяти ...\n");

  while(!(kbhit())) // Выход при нажатии любой клавиши.
  {
    // Прочитать байт из открытого файла.
    fread(buf_out, 1, 1, f_out);

    if (feof(f_out)) // Достигнут конец файла.
    {
      printf("Запись данных в модуль памяти завершена.\n");
      getchar();
      break;
    }
    else
      if (ferror(f_out))
      {
        puts("Ошибка чтения файла w_data_mix.txt.");
        getchar();
        break;
      } 
    
    //buf_out[0] = i; // Переменная-счётчик для тестирования.

    // Вывод данных в COM-порт.
    if (WriteFile(hCom, buf_out, oSize, &BytesWritten, 0))
    {
      if (BytesWritten != 1)
      {
        printf("Ошибка записи байта в COM-порт.\n");
        break;
      }
      else
      {
        // Переданный байт в COM-порт.
        //GoToXY(0, 22);
        //printf("%d %02Xh ", i, buf_out[0]); // Вывод символов на экран ПК занимает время.
        printf("%02Xh ", buf_out[0]);

        i = i + 1; // Увеличить счётчик переданных байтов.
        //Sleep(1); // Пауза 1 мс.

        //GoToXY(0, 23);
        //printf("i = %4Xh", i);

        if (i == 49152) // Завершить запись данных в МП.
        {
          printf("\nЗапись данных в модуль памяти завершена.\n");
          getchar();
          break;
        }
      }
    }
    else
    {
      printf("Ошибка COM-порта при передаче байта.\n");
      getchar();
      return 1;
    }
  }
  fclose(f_out);

  if (key == 't') goto Read_COM;

Quit_COM:

  // Закрыть COM-порт.
  CloseHandle(hCom);
  getchar();
  return 0;
}
