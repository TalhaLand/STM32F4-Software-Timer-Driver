# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde bir Hardware Timer interrupt'ı kullanarak **non-blocking Software Timer** oluşturmayı sağlayan modüler bir C sürücüsüdür.

Driver, örnekte **TIM11** üzerinden 1 ms'lik bir zaman tabanı oluşturur. Bu zaman tabanı kullanılarak tek bir Hardware Timer üzerinden birden fazla bağımsız Software Timer çalıştırılabilir.

`HAL_Delay()` gibi CPU'yu bekleten yöntemler yerine, uygulamanın diğer işlemleri çalışmaya devam ederken belirli sürelerin takip edilmesini sağlar.

---

# 📚 İçindekiler

- [Özellikler](#-özellikler)
- [1. Hızlı Başlangıç](#1--hızlı-başlangıç)
- [2. Driver Dosyalarını Projeye Ekleme](#2--driver-dosyalarını-projeye-ekleme)
- [3. STM32CubeMX Ayarları](#3--stm32cubemx-ayarları)
- [4. Timer Frekansı ve 1 ms Hesabı](#4--timer-frekansı-ve-1-ms-hesabı)
- [5. Driver'ı Başlatma](#5--driverı-başlatma)
- [6. main.c Kullanımı](#6--mainc-kullanımı)
- [7. Driver'ın Çalışma Mantığı](#7--driverın-çalışma-mantığı)
- [8. STimer_t Yapısı](#8--stimer_t-yapısı)
- [9. Driver Fonksiyonları](#9--driver-fonksiyonları)
- [10. Birden Fazla Software Timer](#10--birden-fazla-software-timer)
- [11. Overflow Koruması](#11--overflow-koruması)
- [12. Neden Bu Mimariyi Kullandık?](#12--neden-bu-mimariyi-kullandık)
- [13. Kodu Tekrar Yazmak İçin](#13--kodu-tekrar-yazmak-için)
- [14. Dikkat Edilmesi Gerekenler](#14--dikkat-edilmesi-gerekenler)
- [15. Geliştirilebilecek Kısımlar](#15--geliştirilebilecek-kısımlar)
- [16. Proje Yapısı](#16--proje-yapısı)
- [17. Sonuç](#17--sonuç)

---

# ✨ Özellikler

- Non-blocking Software Timer
- Hardware Timer interrupt tabanlı çalışma
- Milisaniye çözünürlüğünde zamanlama
- Tek Hardware Timer ile birden fazla Software Timer
- `uint32_t` overflow kontrolü
- Basit ve modüler API
- STM32 HAL ile uyumlu yapı

---

# 1. 🚀 Hızlı Başlangıç

Driver'ı kendi STM32CubeIDE projenizde kullanmak için temel olarak şu adımlar yeterlidir:

```text
Software_Timer.c / Software_Timer.h
            ↓
      STM32CubeMX Timer
            ↓
       1 ms Interrupt
            ↓
   #include "Software_Timer.h"
            ↓
        STimer_t
            ↓
   software_Timer_Init()
            ↓
  Software_Timer_Set_Time()
            ↓
Software_Timer_Clock_Check_Elapsed_Time()
```

En basit kullanım:

```c
STimer_t timer;

software_Timer_Init(&htim11);

Software_Timer_Set_Time(&timer, 1000);

while (1)
{
    if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
    {
        // 1 saniye geçti

        Software_Timer_Set_Time(&timer, 1000);
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

Driver'ın çalışabilmesi için bir Hardware Timer'ın **1 ms periyotla interrupt** üretmesi gerekir.

Bu projede örnek olarak **TIM11** kullanılmaktadır.

CubeMX'te:

```text
TIM11 → Enabled
TIM11 Global Interrupt → Enabled
```

olmalıdır.

> **Önemli:** Interrupt aktif değilse `HAL_TIM_PeriodElapsedCallback()` çalışmaz ve `msTick` artmaz.

---

# 4. ⏱️ Timer Frekansı ve 1 ms Hesabı

Bu projedeki örnekte Timer Clock:

```text
50 MHz
```

olarak kullanılmaktadır.

1 ms zaman tabanı için:

```text
Prescaler (PSC) = 49
Counter Period (ARR) = 999
```

kullanılır.

Timer frekansı:

```text
F_interrupt =
F_timer / ((PSC + 1) × (ARR + 1))
```

Hesap:

```text
50,000,000 / ((49 + 1) × (999 + 1))
= 1000 Hz
```

Dolayısıyla:

```text
1 / 1000 = 1 ms
```

elde edilir.

Sonuç:

```text
TIM11 → Her 1 ms'de bir interrupt
```

---

# 5. ▶️ Driver'ı Başlatma

Öncelikle bir Software Timer oluşturulur:

```c
STimer_t timer;
```

Daha sonra CubeMX tarafından oluşturulan Timer handle'ı driver'a gönderilir:

```c
software_Timer_Init(&htim11);
```

Bu fonksiyon Hardware Timer'ı interrupt modunda başlatır.

Temel olarak:

```c
HAL_TIM_Base_Start_IT(htim);
```

fonksiyonunu kullanır.

Ardından Software Timer'a istenen süre verilir:

```c
Software_Timer_Set_Time(&timer, 1000);
```

Burada:

```text
1000 ms = 1 saniye
```

anlamına gelir.

---

# 6. 🧩 main.c Kullanımı

Tam bir kullanım örneği:

```c
#include "main.h"
#include "Software_Timer.h"

STimer_t ledTimer;

int main(void)
{
    HAL_Init();

    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM11_Init();

    software_Timer_Init(&htim11);

    Software_Timer_Set_Time(&ledTimer, 1000);

    while (1)
    {
        if (Software_Timer_Clock_Check_Elapsed_Time(&ledTimer))
        {
            HAL_GPIO_TogglePin(
                GPIOA,
                GPIO_PIN_5
            );

            Software_Timer_Set_Time(
                &ledTimer,
                1000
            );
        }
    }
}
```

Bu örnekte LED her 1 saniyede bir toggle edilir.

Timer'ın çalışması için `while(1)` içerisinde herhangi bir `HAL_Delay()` kullanılmasına gerek yoktur.

---

# 7. 🧠 Driver'ın Çalışma Mantığı

Driver'ın temelinde `msTick` isimli milisaniye zaman sayacı bulunur.

TIM11 her 1 ms'de bir interrupt oluşturduğunda:

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM11)
    {
        msTick++;
    }
}
```

çalışır.

Böylece:

```text
TIM11
  ↓
1 ms Interrupt
  ↓
msTick++
  ↓
Software Timer
  ↓
Geçen süreyi kontrol et
  ↓
Süre doldu → true
```

şeklinde bir akış oluşur.

Software Timer başlatıldığında mevcut `msTick` değeri `startTime` olarak kaydedilir.

Daha sonra:

```text
currentTime - startTime
```

hesaplanarak geçen süre bulunur.

Geçen süre `intervalTime` değerine ulaştığında timer süresi dolmuş kabul edilir.

---

# 8. 📦 STimer_t Yapısı

Her Software Timer için:

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

yapısı kullanılır.

### `startTime`

Timer'ın başlatıldığı andaki `msTick` değeridir.

### `intervalTime`

Timer'ın beklemesi gereken süreyi milisaniye cinsinden tutar.

### `activated`

Timer'ın aktif olup olmadığını belirtir.

```text
true  → Aktif
false → Pasif
```

Örneğin:

```c
STimer_t timer;

Software_Timer_Set_Time(&timer, 500);
```

çağrıldığında timer 500 ms süre için aktif hale gelir.

---

# 9. 🔧 Driver Fonksiyonları

## `software_Timer_Init()`

```c
void software_Timer_Init(TIM_HandleTypeDef *htim);
```

Hardware Timer'ı interrupt modunda başlatır.

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

Software Timer'ı başlatır veya yeniden kurar.

Yaptığı temel işlemler:

```text
startTime    → Mevcut msTick
intervalTime → İstenen süre
activated    → true
```

Örnek:

```c
Software_Timer_Set_Time(&timer, 1000);
```

---

## `Software_Timer_Get_Time()`

```c
uint32_t Software_Timer_Get_Time(void);
```

Güncel `msTick` değerini döndürür.

Driver içerisinde zaman bilgisini almak için kullanılır.

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
true
```

döndürür.

Süre dolmadığında:

```c
false
```

döndürür.

Süre dolduğunda timer pasif hale getirilir.

Bu nedenle periyodik kullanım için timer'ın tekrar kurulması gerekir:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // Görev

    Software_Timer_Set_Time(&timer, 1000);
}
```

---

## `Software_Timer_Disable()`

```c
void Software_Timer_Disable(STimer_t *STimer);
```

İlgili Software Timer'ı pasif hale getirir.

```c
Software_Timer_Disable(&timer);
```

Bu işlem Hardware Timer'ı durdurmaz. Sadece ilgili Software Timer'ı devre dışı bırakır.

---

# 10. 🔢 Birden Fazla Software Timer

Tek bir Hardware Timer üzerinden birden fazla Software Timer oluşturulabilir.

Örneğin:

```c
STimer_t timer_1s;
STimer_t timer_500ms;

Software_Timer_Set_Time(&timer_1s, 1000);
Software_Timer_Set_Time(&timer_500ms, 500);
```

Daha sonra:

```c
while (1)
{
    if (Software_Timer_Clock_Check_Elapsed_Time(&timer_1s))
    {
        // 1 saniyelik görev

        Software_Timer_Set_Time(
            &timer_1s,
            1000
        );
    }

    if (Software_Timer_Clock_Check_Elapsed_Time(&timer_500ms))
    {
        // 500 ms'lik görev

        Software_Timer_Set_Time(
            &timer_500ms,
            500
        );
    }
}
```

şeklinde kullanılabilir.

Her timer aynı `msTick` zaman tabanını kullanırken kendi `startTime` ve `intervalTime` değerlerine sahip olur.

---

# 11. 🔄 Overflow Koruması

`msTick` değişkeni `uint32_t` olduğu için maksimum:

```text
4,294,967,295
```

değerine ulaşabilir.

1 ms çözünürlükte yaklaşık **49.7 gün** sonra tekrar `0` değerine döner.

```text
4,294,967,295
        ↓
        0
        ↓
        1
        ↓
        2
```

Driver içerisindeki elapsed-time kontrolü bu overflow durumunu ayrıca ele alır.

Böylece `msTick` sıfıra döndüğünde timer hesabının bozulması önlenir.

Temel amaç:

```text
Normal çalışma
      +
Overflow durumu
      ↓
Doğru geçen süre hesabı
```

şeklindedir.

---

# 12. 💡 Neden Bu Mimariyi Kullandık?

### Non-blocking çalışma

`HAL_Delay()` kullanıldığında uygulama belirli bir süre beklemek zorunda kalır.

Software Timer ile:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // Görev
}
```

şeklinde kontrol yapılır ve diğer işlemler çalışmaya devam edebilir.

### Birden fazla timer

Her görev için ayrı Hardware Timer kullanmak yerine tek bir Hardware Timer'ın oluşturduğu `msTick` üzerinden birden fazla Software Timer oluşturulabilir.

### Modüler yapı

Uygulama kodu doğrudan Timer register'larıyla uğraşmaz.

Sadece:

```c
Software_Timer_Set_Time()
Software_Timer_Clock_Check_Elapsed_Time()
Software_Timer_Disable()
```

gibi fonksiyonlarla zamanlayıcıyı kullanır.

---

# 13. 🛠️ Kodu Tekrar Yazmak İçin

Bu driver'ı ileride sıfırdan yazmak istersen temel mantığı şu şekilde hatırlayabilirsin:

### 1. Hardware Timer'ı 1 ms interrupt verecek şekilde ayarla

```text
TIM11
  ↓
1 ms Interrupt
```

### 2. Global zaman sayacı oluştur

```c
uint32_t msTick = 0;
```

### 3. Her interrupt'ta artır

```c
msTick++;
```

### 4. Timer bilgilerini struct içerisinde tut

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

### 5. Timer başlatılırken başlangıç zamanını kaydet

```text
startTime = msTick
```

### 6. Geçen zamanı kontrol et

```text
currentTime - startTime
```

değerini `intervalTime` ile karşılaştır.

### 7. Süre dolduysa timer'ı pasifleştir

```text
Süre doldu
    ↓
Disable
    ↓
true
```

### 8. Periyodik görev gerekiyorsa yeniden kur

```c
Software_Timer_Set_Time(&timer, 1000);
```

Özet:

```text
TIM11
  ↓
1 ms Interrupt
  ↓
msTick++
  ↓
Set Time
  ↓
Elapsed Time Check
  ↓
Task
```

---

# 14. ⚠️ Dikkat Edilmesi Gerekenler

- Timer interrupt'ı CubeMX üzerinden aktif edilmelidir.
- Hardware Timer'ın periyodu driver'ın kullandığı zaman tabanına uygun olmalıdır.
- `software_Timer_Init()` çağrılmadan Software Timer kullanılmamalıdır.
- Periyodik timer için süre dolduktan sonra `Software_Timer_Set_Time()` tekrar çağrılmalıdır.
- `Software_Timer_Disable()` Hardware Timer'ı değil, yalnızca ilgili Software Timer'ı durdurur.
- `uint32_t` overflow durumu göz önünde bulundurulmalıdır.

---

# 15. 🔮 Geliştirilebilecek Kısımlar

Driver temel bir Software Timer altyapısı olarak tasarlanmıştır.

İleride:

- Otomatik periyodik timer modu
- Callback desteği
- Mikro saniye çözünürlüğü
- Tek seferlik / periyodik timer seçimi
- Daha gelişmiş timer yönetimi

gibi özellikler eklenebilir.

---

# 16. 📁 Proje Yapısı

```text
STM32F4-Software-Timer-Driver/
│
├── Software_Timer.c
├── Software_Timer.h
├── main.c
├── main.h
└── README.md
```

### `Software_Timer.h`

Timer veri yapısını ve driver fonksiyonlarının prototiplerini içerir.

### `Software_Timer.c`

Software Timer'ın çalışma mantığını ve zaman kontrol fonksiyonlarını içerir.

### `main.c`

Driver'ın STM32 uygulaması içerisinde nasıl kullanılabileceğini gösterir.

---

# 17. 🎯 Sonuç

Bu driver, STM32F4 üzerinde tek bir Hardware Timer interrupt'ını kullanarak birden fazla **non-blocking Software Timer** oluşturmayı sağlar.

Temel kullanım:

```c
STimer_t timer;

software_Timer_Init(&htim11);

Software_Timer_Set_Time(&timer, 1000);

while (1)
{
    if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
    {
        // 1 saniye geçti

        Software_Timer_Set_Time(&timer, 1000);
    }
}
```

Temel çalışma mantığı:

```text
Hardware Timer
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

Bu yapı sayesinde STM32 uygulamalarında `HAL_Delay()` kullanmadan farklı görevlerin belirli zaman aralıklarında çalıştırılması sağlanabilir.

---

## 👨‍💻 Author

**Talha Mansur Şahin**

STM32 | Embedded Systems | C
