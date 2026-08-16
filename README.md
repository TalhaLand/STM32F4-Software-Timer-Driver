# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde bir Hardware Timer interrupt'ı kullanarak **non-blocking (engellemesiz) Software Timer** oluşturmayı sağlayan modüler bir C sürücüsüdür.

Driver, örnekte **TIM11** üzerinden 1 ms'lik bir zaman tabanı (`msTick`) oluşturur. Bu zaman tabanı kullanılarak aynı Hardware Timer üzerinden birden fazla bağımsız Software Timer oluşturulabilir.

`HAL_Delay()` gibi CPU'yu bekleten yöntemler yerine, uygulamanın diğer işlemleri çalışmaya devam ederken belirli sürelerin takip edilmesini sağlar.

---

# 📚 İçindekiler

- [Özellikler](#-özellikler)
- [1. Hızlı Başlangıç](#1--hızlı-başlangıç)
- [2. Driver Dosyalarını Projeye Ekleme](#2--driver-dosyalarını-projeye-ekleme)
- [3. STM32CubeMX Ayarları](#3--stm32cubemx-ayarları)
- [4. Timer Frekansı ve 1 ms Hesabı](#4--timer-frekansı-ve-1-ms-hesabı)
- [5. main.c İçerisinde Kullanım](#5--mainc-içerisinde-kullanım)
- [6. Timer Başlatma](#6--timer-başlatma)
- [7. Bir Software Timer Oluşturma](#7--bir-software-timer-oluşturma)
- [8. Timer Süresini Ayarlama](#8--timer-süresini-ayarlama)
- [9. Sürenin Dolup Dolmadığını Kontrol Etme](#9--sürenin-dolup-dolmadığını-kontrol-etme)
- [10. Timer'ı Durdurma](#10--timerı-durdurma)
- [11. Birden Fazla Software Timer Kullanımı](#11--birden-fazla-software-timer-kullanımı)
- [12. Interrupt ve msTick Mantığı](#12--interrupt-ve-mstick-mantığı)
- [13. Software Timer'ın Çalışma Mantığı](#13--software-timerın-çalışma-mantığı)
- [14. STimer_t Yapısı](#14--stimer_t-yapısı)
- [15. uint32_t Overflow Koruması](#15--uint32_t-overflow-koruması)
- [16. Driver Fonksiyonları](#16--driver-fonksiyonları)
- [17. Neden HAL_Delay Yerine Software Timer?](#17--neden-hal_delay-yerine-software-timer)
- [18. Tam Kullanım Örneği](#18--tam-kullanım-örneği)
- [19. Driver'ı Sıfırdan Tekrar Yazmak](#19--driverı-sıfırdan-tekrar-yazmak)
- [20. Neden Bu Mimariyi Kullandık?](#20--neden-bu-mimariyi-kullandık)
- [21. Dikkat Edilmesi Gerekenler](#21--dikkat-edilmesi-gerekenler)
- [22. Geliştirilebilecek Kısımlar](#22--geliştirilebilecek-kısımlar)
- [23. Proje Yapısı](#23--proje-yapısı)
- [24. 6 Ay Sonra Hatırlamak İçin](#24--6-ay-sonra-hatırlamak-için)
- [25. Sonuç](#25--sonuç)

---

# 🚀 Özellikler

- Non-blocking Software Timer
- Hardware Timer interrupt tabanlı çalışma
- Milisaniye çözünürlüğünde zamanlama
- Tek Hardware Timer ile birden fazla Software Timer
- `uint32_t` overflow kontrolü
- Timer'ın aktif/pasif durumunu takip etme
- Basit ve modüler API
- STM32 HAL ile uyumlu yapı

---

# 1. 🚀 Hızlı Başlangıç

Driver'ı başka bir STM32CubeIDE projesinde kullanmak için temel olarak:

```text
Software_Timer.c
Software_Timer.h
        │
        ▼
STM32CubeMX üzerinde TIM11
        │
        ▼
1 ms Hardware Timer Interrupt
        │
        ▼
#include "Software_Timer.h"
        │
        ▼
STimer_t oluştur
        │
        ▼
software_Timer_Init()
        │
        ▼
Software_Timer_Set_Time()
        │
        ▼
Software_Timer_Clock_Check_Elapsed_Time()
```

şeklinde ilerlenir.

En basit kullanım:

```c
STimer_t stimer;

software_Timer_Init(&htim11);

Software_Timer_Set_Time(&stimer, 1000);

while (1)
{
    if (Software_Timer_Clock_Check_Elapsed_Time(&stimer))
    {
        // 1 saniye geçti

        Software_Timer_Set_Time(&stimer, 1000);
    }
}
```

---

# 2. 📁 Driver Dosyalarını Projeye Ekleme

Repository içerisindeki:

```text
Software_Timer.c
Software_Timer.h
```

dosyalarını STM32CubeIDE projenize ekleyin.

Örneğin:

```text
MyProject/
│
├── Core/
│   ├── Inc/
│   │   └── Software_Timer.h
│   │
│   └── Src/
│       └── Software_Timer.c
│
└── ...
```

Daha sonra driver'ı kullanacağınız `.c` dosyasına:

```c
#include "Software_Timer.h"
```

ekleyin.

---

# 3. ⚙️ STM32CubeMX Ayarları

Driver'ın çalışması için bir Hardware Timer'ın belirli bir periyotla interrupt üretmesi gerekir.

Bu repository'deki örnekte:

```text
Hardware Timer → TIM11
Timer Period   → 1 ms
Interrupt      → Enabled
```

olarak yapılandırılmıştır.

CubeMX içerisinde:

### TIM11

```text
TIM11 → Activated
```

olmalıdır.

### NVIC

```text
TIM11 global interrupt → Enabled
```

olmalıdır.

> **Önemli:** Timer interrupt aktif değilse `HAL_TIM_PeriodElapsedCallback()` çalışmaz ve `msTick` artmaz.

---

# 4. ⏱️ Timer Frekansı ve 1 ms Hesabı

Bu projedeki örnekte Timer Clock:

```text
50 MHz
```

olarak kullanılmaktadır.

1 ms'lik interrupt elde etmek için:

```text
Prescaler (PSC) = 49
Counter Period (ARR) = 999
```

kullanılmıştır.

Timer interrupt frekansı:

```text
F_interrupt =
F_timer / ((PSC + 1) × (ARR + 1))
```

Yerine koyarsak:

```text
F_interrupt =
50,000,000 / ((49 + 1) × (999 + 1))

F_interrupt =
50,000,000 / 50,000

F_interrupt =
1000 Hz
```

1000 Hz'in periyodu:

```text
1 / 1000 = 0.001 s

0.001 s = 1 ms
```

Dolayısıyla:

```text
TIM11 → Her 1 ms'de bir interrupt
```

oluşturur.

---

# 5. 🧩 main.c İçerisinde Kullanım

Öncelikle:

```c
#include "main.h"
#include "Software_Timer.h"
```

eklenir.

Daha sonra bir Software Timer oluşturulur:

```c
STimer_t stimer_1s;
```

`main()` içerisinde Hardware Timer başlatılır:

```c
software_Timer_Init(&htim11);
```

Ardından Software Timer'a süre verilir:

```c
Software_Timer_Set_Time(&stimer_1s, 1000);
```

Buradaki:

```text
1000 ms = 1 saniye
```

anlamına gelir.

---

# 6. ▶️ Timer Başlatma

Driver'ın Hardware Timer'ı interrupt modunda başlatan fonksiyonu:

```c
software_Timer_Init(&htim11);
```

şeklindedir.

Fonksiyon içerisinde:

```c
HAL_TIM_Base_Start_IT(htim);
```

çağrılır.

Yani:

```text
software_Timer_Init()
        │
        ▼
HAL_TIM_Base_Start_IT()
        │
        ▼
Hardware Timer Interrupt
        │
        ▼
msTick++
```

akışı oluşur.

---

# 7. ⏲️ Bir Software Timer Oluşturma

Bir Software Timer oluşturmak için:

```c
STimer_t stimer_1s;
```

yeterlidir.

Başka timerlar oluşturmak için:

```c
STimer_t stimer_500ms;
STimer_t stimer_2s;
STimer_t stimer_5s;
```

gibi farklı timer nesneleri oluşturulabilir.

Hepsi aynı `msTick` zaman tabanını kullanır.

---

# 8. 🕐 Timer Süresini Ayarlama

Timer'a süre vermek için:

```c
Software_Timer_Set_Time(
    &stimer_1s,
    1000
);
```

kullanılır.

Buradaki ikinci parametre milisaniye cinsindendir.

Örnekler:

```c
Software_Timer_Set_Time(&stimer, 100);
```

→ 100 ms

```c
Software_Timer_Set_Time(&stimer, 500);
```

→ 500 ms

```c
Software_Timer_Set_Time(&stimer, 1000);
```

→ 1 saniye

```c
Software_Timer_Set_Time(&stimer, 5000);
```

→ 5 saniye

Fonksiyon çağrıldığında timer'ın başlangıç zamanı:

```c
STimer->startTime =
    Software_Timer_Get_Time();
```

ile kaydedilir.

Aynı zamanda istenen süre:

```c
STimer->intervalTime =
    intervalMs;
```

olarak saklanır ve timer aktif edilir:

```c
STimer->activated = true;
```

---

# 9. ✅ Sürenin Dolup Dolmadığını Kontrol Etme

Timer'ın süresinin dolup dolmadığını:

```c
Software_Timer_Clock_Check_Elapsed_Time()
```

fonksiyonu kontrol eder.

Örneğin:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&stimer_1s))
{
    HAL_GPIO_TogglePin(
        GPIOA,
        GPIO_PIN_5
    );

    Software_Timer_Set_Time(
        &stimer_1s,
        1000
    );
}
```

Timer'ın süresi dolduğunda fonksiyon:

```c
true
```

döndürür.

Süre dolmamışsa:

```c
false
```

döndürür.

---

# 10. 🛑 Timer'ı Durdurma

Bir Software Timer'ı pasif hale getirmek için:

```c
Software_Timer_Disable(&stimer);
```

kullanılır.

Fonksiyon:

```c
STimer->activated = false;
```

yapar.

Önemli nokta:

Bu işlem Hardware Timer'ı durdurmaz.

Sadece ilgili **Software Timer'ı pasif hale getirir.**

---

# 11. 🔢 Birden Fazla Software Timer Kullanımı

Driver'ın önemli özelliklerinden biri aynı Hardware Timer üzerinden birden fazla Software Timer oluşturabilmesidir.

Örneğin:

```c
STimer_t stimer_1s;
STimer_t stimer_500ms;
```

oluşturabiliriz.

Başlatma:

```c
Software_Timer_Set_Time(
    &stimer_1s,
    1000
);

Software_Timer_Set_Time(
    &stimer_500ms,
    500
);
```

Ana döngü:

```c
while (1)
{
    if (
        Software_Timer_Clock_Check_Elapsed_Time(
            &stimer_1s
        )
    )
    {
        // 1 saniyelik görev

        Software_Timer_Set_Time(
            &stimer_1s,
            1000
        );
    }

    if (
        Software_Timer_Clock_Check_Elapsed_Time(
            &stimer_500ms
        )
    )
    {
        // 500 ms'lik görev

        Software_Timer_Set_Time(
            &stimer_500ms,
            500
        );
    }
}
```

Burada:

```text
TIM11
  │
  └── msTick
       │
       ├── stimer_1s
       │
       └── stimer_500ms
```

şeklinde tek bir zaman tabanından birden fazla Software Timer üretilir.

---

# 12. ⚡ Interrupt ve msTick Mantığı

Driver'ın temelini `msTick` değişkeni oluşturur.

```c
uint32_t msTick = 0;
```

TIM11 her 1 ms'de bir interrupt oluşturduğunda:

```c
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim
)
{
    if (htim->Instance == TIM11)
    {
        msTick++;
    }
}
```

çalışır.

Dolayısıyla:

```text
1 ms      → msTick = 1
2 ms      → msTick = 2
3 ms      → msTick = 3
...
1000 ms   → msTick = 1000
```

şeklinde ilerler.

Bu değişken Software Timer'ların kullandığı temel zaman referansıdır.

---

# 13. 🧠 Software Timer'ın Çalışma Mantığı

Bir Software Timer oluşturulduğunda iki önemli bilgi saklanır:

```text
startTime
intervalTime
```

Örneğin:

```c
Software_Timer_Set_Time(
    &stimer,
    1000
);
```

çağrıldığında:

```text
startTime    = O anki msTick
intervalTime = 1000 ms
activated    = true
```

olur.

Daha sonra:

```c
Software_Timer_Clock_Check_Elapsed_Time()
```

çağrıldığında güncel `msTick` alınır.

Temel kontrol:

```text
currentTick - startTime
        │
        ▼
    Geçen süre
        │
        ▼
 intervalTime ile karşılaştır
        │
   ┌────┴────┐
   ▼         ▼
 Süre doldu  Dolmadı
   │         │
 true       false
```

---

# 14. 📦 STimer_t Yapısı

Driver'ın her Software Timer için tuttuğu bilgiler:

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

şeklindedir.

### `startTime`

Timer'ın başlatıldığı andaki `msTick` değeridir.

### `intervalTime`

Timer'ın beklemesi gereken süreyi milisaniye cinsinden tutar.

### `activated`

Timer'ın aktif olup olmadığını belirtir.

```text
true  → Timer aktif
false → Timer pasif
```

---

# 15. 🔄 uint32_t Overflow Koruması

`msTick`:

```c
uint32_t
```

olduğu için maksimum değeri:

```text
4,294,967,295
```

olur.

1 ms çözünürlükte bu değer yaklaşık:

```text
49.7 gün
```

sonra tekrar `0` değerine döner.

Örneğin:

```text
4,294,967,293
4,294,967,294
4,294,967,295
0
1
2
3
```

şeklinde devam eder.

Driver bu nedenle başlangıç zamanı ile güncel zaman arasındaki ilişkiyi kontrol ederek overflow durumunu da ele alır.

Böylece `msTick` sıfıra döndüğünde Software Timer mantığının bozulması önlenir.

---

# 16. 🔧 Driver Fonksiyonları

## `software_Timer_Init()`

```c
void software_Timer_Init(
    TIM_HandleTypeDef *htim
);
```

Hardware Timer'ı interrupt modunda başlatır.

İçerisinde:

```c
HAL_TIM_Base_Start_IT(htim);
```

kullanılır.

Kullanım:

```c
software_Timer_Init(&htim11);
```

---

## `Software_Timer_Set_Time()`

```c
void Software_Timer_Set_Time(
    STimer_t *STimer,
    uint32_t intervalMs
);
```

Software Timer'ı kurar.

Yaptığı işlemler:

```text
startTime    → mevcut msTick
intervalTime → istenen süre
activated    → true
```

Kullanım:

```c
Software_Timer_Set_Time(
    &stimer,
    1000
);
```

---

## `Software_Timer_Get_Time()`

```c
uint32_t Software_Timer_Get_Time(void);
```

Güncel `msTick` değerini döndürür.

Temel olarak:

```c
return msTick;
```

işlemini gerçekleştirir.

---

## `Software_Timer_Clock_Check_Elapsed_Time()`

```c
bool Software_Timer_Clock_Check_Elapsed_Time(
    STimer_t *STimer
);
```

Timer'ın süresinin dolup dolmadığını kontrol eder.

Süre dolduğunda:

```c
return true;
```

Süre dolmadığında:

```c
return false;
```

döndürür.

---

## `Software_Timer_Disable()`

```c
void Software_Timer_Disable(
    STimer_t *STimer
);
```

ilgili Software Timer'ı pasif hale getirir:

```c
STimer->activated = false;
```

---

# 17. 🚫 Neden HAL_Delay Yerine Software Timer?

`HAL_Delay()` kullanıldığında:

```c
HAL_Delay(1000);
```

CPU bekleme süresine girer.

Software Timer yaklaşımında ise:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // İşlem
}
```

kontrolü yapılır.

Timer henüz dolmadıysa `while(1)` içerisindeki diğer kodlar çalışmaya devam eder.

Örneğin:

```text
while(1)
{
    Timer kontrolü
    UART kontrolü
    Button kontrolü
    Sensor kontrolü
}
```

gibi birçok işlem aynı döngü içerisinde yürütülebilir.

Bu nedenle yapı **non-blocking** olarak adlandırılır.

---

# 18. 🧪 Tam Kullanım Örneği

Aşağıdaki örnekte iki farklı Software Timer kullanılmaktadır:

```c
#include "main.h"
#include "Software_Timer.h"

STimer_t stimer_1s;
STimer_t stimer_500ms;

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM11_Init();

    /*
     * Hardware Timer'ı başlat
     */
    software_Timer_Init(&htim11);

    /*
     * Software Timer'ları kur
     */
    Software_Timer_Set_Time(
        &stimer_1s,
        1000
    );

    Software_Timer_Set_Time(
        &stimer_500ms,
        500
    );

    while (1)
    {
        /*
         * 1 saniyelik timer
         */
        if (
            Software_Timer_Clock_Check_Elapsed_Time(
                &stimer_1s
            )
        )
        {
            HAL_GPIO_TogglePin(
                GPIOA,
                GPIO_PIN_5
            );

            /*
             * Timer'ı tekrar kur
             */
            Software_Timer_Set_Time(
                &stimer_1s,
                1000
            );
        }

        /*
         * 500 ms'lik timer
         */
        if (
            Software_Timer_Clock_Check_Elapsed_Time(
                &stimer_500ms
            )
        )
        {
            /*
             * Burada farklı bir işlem yapılabilir.
             */

            Software_Timer_Set_Time(
                &stimer_500ms,
                500
            );
        }
    }
}
```

---

# 19. 🛠️ Driver'ı Sıfırdan Tekrar Yazmak

Bu driver'ı ileride tekrar yazmak istersen temel mantık şu sırayla kurulabilir:

### 1. Hardware Timer oluştur

Örneğin:

```text
TIM11
```

### 2. Timer'ı 1 ms interrupt verecek şekilde ayarla

```text
50 MHz
PSC = 49
ARR = 999
```

### 3. Global tick oluştur

```c
uint32_t msTick = 0;
```

### 4. Timer callback oluştur

```c
void HAL_TIM_PeriodElapsedCallback(
    TIM_HandleTypeDef *htim
)
{
    if (htim->Instance == TIM11)
    {
        msTick++;
    }
}
```

### 5. Software Timer yapısını oluştur

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

### 6. Timer başlatma fonksiyonu oluştur

```c
software_Timer_Init()
```

### 7. Timer kurma fonksiyonu oluştur

```c
Software_Timer_Set_Time()
```

### 8. Geçen zamanı kontrol et

```c
Software_Timer_Clock_Check_Elapsed_Time()
```

### 9. Overflow durumunu kontrol et

```text
Normal durum
+
Overflow durumu
```

### 10. Timer'ı gerektiğinde pasifleştir

```c
Software_Timer_Disable()
```

Bu temel yapı Software Timer'ın çalışma mantığını oluşturur.

---

# 20. 🧠 Neden Bu Mimariyi Kullandık?

Bu driver'da Hardware Timer yalnızca **zaman tabanı oluşturmak** için kullanılır.

Hardware Timer:

```text
TIM11
  │
  ▼
1 ms Interrupt
  │
  ▼
msTick++
```

Software Timer ise bu zaman bilgisini kullanır:

```text
msTick
  │
  ├── Timer 1 → 100 ms
  ├── Timer 2 → 500 ms
  ├── Timer 3 → 1 s
  └── Timer 4 → 5 s
```

Böylece her görev için ayrı bir Hardware Timer kullanmak gerekmez.

> **Tek bir Hardware Timer ile birden fazla bağımsız zamanlayıcı oluşturulabilir.**

---

# 21. ⚠️ Dikkat Edilmesi Gerekenler

### Timer Interrupt aktif olmalı

CubeMX'te:

```text
TIM11 global interrupt
```

etkin olmalıdır.

Aksi halde:

```c
msTick++;
```

çalışmaz.

### Timer periyodu 1 ms olmalı

Bu driver'ın zaman tabanı 1 ms olarak tasarlanmıştır.

Timer periyodunu değiştirirseniz `msTick` değerinin temsil ettiği zaman da değişir.

### Aynı timer kullanılmalı

Driver başlatılırken:

```c
software_Timer_Init(&htim11);
```

ile kullanılan Hardware Timer'ın callback içerisinde kontrol edilen timer ile aynı olması gerekir.

### Periyodik timer için yeniden kurulmalı

Timer'ın süresi dolduğunda tekrar çalışmasını istiyorsanız:

```c
Software_Timer_Set_Time(
    &timer,
    interval
);
```

ile yeniden kurulmalıdır.

---

# 22. 🔮 Geliştirilebilecek Kısımlar

Mevcut driver temel bir Software Timer altyapısıdır.

İleride şu özellikler eklenebilir:

- Otomatik periyodik timer modu
- Callback desteği
- Timer başlat/durdur API'sinin genişletilmesi
- Mikro saniye çözünürlüğü
- Tek seferlik / periyodik timer ayrımı
- Timer ID sistemi
- Timer listesi
- RTOS ile kullanım
- Daha gelişmiş timer yönetimi

---

# 23. 📁 Proje Yapısı

Repository'nin temel yapısı:

```text
STM32F4-Software-Timer-Driver/
│
├── Software_Timer.c
├── Software_Timer.h
├── main.c
├── main.h
├── README.md
│
└── ...
```

Driver'ın temel dosyaları:

```text
Software_Timer.c
Software_Timer.h
```

şeklindedir.

---

# 24. 🧠 6 Ay Sonra Hatırlamak İçin

Bu driver'ın bütün mantığını tekrar hatırlamak için şu zinciri düşün:

```text
TIM11
  ↓
1 ms Interrupt
  ↓
msTick++
  ↓
Software_Timer_Set_Time()
  ↓
startTime kaydedilir
  ↓
while(1)
  ↓
Software_Timer_Clock_Check_Elapsed_Time()
  ↓
currentTick - startTime
  ↓
Süre doldu mu?
  ↓
 ┌──────────────┐
 │              │
Hayır          Evet
 │              │
 ▼              ▼
Bekle      Timer süresi doldu
                │
                ▼
          Görevi çalıştır
                │
                ▼
          Timer'ı yeniden kur
```

En kısa haliyle:

```text
Hardware Timer
      ↓
   1 ms tick
      ↓
   msTick++
      ↓
Software Timer
      ↓
Geçen zamanı kontrol et
      ↓
Süre doldu mu?
      ↓
Evet → Görevi çalıştır
```

Overflow için:

```text
uint32_t msTick
      ↓
49.7 gün
      ↓
0'a taşar
      ↓
Overflow kontrolü
      ↓
Timer çalışmaya devam eder
```

---

# 25. 🎯 Sonuç

Bu driver, STM32F4 üzerinde tek bir Hardware Timer interrupt'ını kullanarak birden fazla **non-blocking Software Timer** oluşturmayı sağlar.

Temel kullanım:

```c
STimer_t timer;

software_Timer_Init(&htim11);

Software_Timer_Set_Time(
    &timer,
    1000
);

while (1)
{
    if (
        Software_Timer_Clock_Check_Elapsed_Time(
            &timer
        )
    )
    {
        // 1 saniye geçti

        Software_Timer_Set_Time(
            &timer,
            1000
        );
    }
}
```

Driver'ın temel çalışma mantığı:

```text
TIM11
  ↓
1 ms Interrupt
  ↓
msTick++
  ↓
Software Timer
  ↓
Elapsed Time Check
  ↓
Task
```

Bu yapı sayesinde `HAL_Delay()` kullanmadan birden fazla görevin farklı zaman aralıklarında çalıştırılması sağlanabilir.

---

## 👨‍💻 Author

**Talha Mansur Şahin**

STM32 / Embedded Systems / C
