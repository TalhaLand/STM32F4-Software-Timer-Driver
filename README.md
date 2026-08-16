# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde bir Hardware Timer interrupt'ı kullanarak **non-blocking Software Timer** oluşturmayı sağlayan modüler bir C sürücüsüdür.

Driver, 1 ms'lik bir zaman tabanı oluşturarak tek bir Hardware Timer üzerinden birden fazla bağımsız Software Timer çalıştırılmasına olanak sağlar.

`HAL_Delay()` gibi CPU'yu bekleten yöntemler yerine, uygulamanın diğer işlemleri çalışmaya devam ederken belirli sürelerin takip edilmesini sağlar.

---

# 📚 İçindekiler

- [Özellikler](#-özellikler)
- [Hızlı Başlangıç](#-hızlı-başlangıç)
- [1. Driver Dosyalarını Projeye Ekleme](#1--driver-dosyalarını-projeye-ekleme)
- [2. STM32CubeMX Ayarları](#2--stm32cubemx-ayarları)
- [3. Timer Frekansı ve 1 ms Hesabı](#3--timer-frekansı-ve-1-ms-hesabı)
- [4. Driver'ı Başlatma](#4--driverı-başlatma)
- [5. main.c Kullanımı](#5--mainc-kullanımı)
- [Driver'ın Çalışma Mantığı](#-driverın-çalışma-mantığı)
- [STimer_t Yapısı](#-stimer_t-yapısı)
- [Driver Fonksiyonları](#-driver-fonksiyonları)
- [Birden Fazla Software Timer](#-birden-fazla-software-timer)
- [Overflow Koruması](#-overflow-koruması)
- [Neden Software Timer?](#-neden-software-timer)
- [Kodu Tekrar Yazmak İçin](#-kodu-tekrar-yazmak-için)
- [Dikkat Edilmesi Gerekenler](#️-dikkat-edilmesi-gerekenler)
- [Geliştirilebilecek Kısımlar](#-geliştirilebilecek-kısımlar)
- [Proje Yapısı](#-proje-yapısı)
- [Sonuç](#-sonuç)

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

# 🚀 Hızlı Başlangıç

Driver'ı kullanmak için temel olarak:

```text
Software_Timer.c / Software_Timer.h
              ↓
       Hardware Timer
              ↓
        1 ms Interrupt
              ↓
          msTick
              ↓
       STimer_t oluştur
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

# 1. 📁 Driver Dosyalarını Projeye Ekleme

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

# 2. ⚙️ STM32CubeMX Ayarları

Driver'ın çalışabilmesi için bir Hardware Timer'ın düzenli olarak interrupt üretmesi gerekir.

Bu projede örnek olarak **TIM11** kullanılmaktadır.

CubeMX üzerinde:

```text
TIM11 → Enabled
TIM11 Global Interrupt → Enabled
```

olmalıdır.

Ayrıca NVIC üzerinden TIM11 interrupt'ının aktif olduğundan emin olun.

> **Önemli:** Timer interrupt'ı çalışmazsa `msTick` güncellenmez ve Software Timer çalışmaz.

---

# 3. ⏱️ Timer Frekansı ve 1 ms Hesabı

Bu projedeki örnek yapılandırmada Timer Clock:

```text
50 MHz
```

olarak kullanılmaktadır.

1 ms'lik zaman tabanı için:

```text
Prescaler (PSC) = 49
Counter Period (ARR) = 999
```

değerleri kullanılabilir.

Timer interrupt frekansı:

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

> Bu değerler örnek Timer Clock yapılandırmasına göredir. Farklı bir clock frekansında PSC ve ARR değerleri yeniden hesaplanmalıdır.

---

# 4. ▶️ Driver'ı Başlatma

Öncelikle bir Software Timer oluşturulur:

```c
STimer_t timer;
```

Daha sonra Hardware Timer driver'a gönderilir:

```c
software_Timer_Init(&htim11);
```

Bu fonksiyon Timer'ı interrupt modunda başlatır.

Temel olarak:

```c
HAL_TIM_Base_Start_IT(htim);
```

kullanılır.

Ardından timer'a istenen süre verilir:

```c
Software_Timer_Set_Time(&timer, 1000);
```

Burada:

```text
1000 ms = 1 saniye
```

anlamına gelir.

---

# 5. 🧩 main.c Kullanımı

Aşağıdaki örnekte LED'in her 1 saniyede bir toggle edilmesi gösterilmiştir:

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
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

            Software_Timer_Set_Time(&ledTimer, 1000);
        }
    }
}
```

Bu örnekte:

```text
TIM11
  ↓
1 ms interrupt
  ↓
msTick
  ↓
1000 ms
  ↓
LED Toggle
```

şeklinde bir akış oluşur.

`HAL_Delay()` kullanılmadığı için `while(1)` içerisindeki diğer işlemler çalışmaya devam edebilir.

---

# 🧠 Driver'ın Çalışma Mantığı

Driver'ın temelinde milisaniye cinsinden çalışan `msTick` zaman sayacı bulunur.

Timer her 1 ms'de bir interrupt oluşturduğunda zaman sayacı artırılır:

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM11)
    {
        msTick++;
    }
}
```

Böylece:

```text
TIM11
  ↓
1 ms Interrupt
  ↓
msTick++
```

şeklinde sürekli ilerleyen bir zaman tabanı oluşur.

Software Timer başlatılırken mevcut zaman `startTime` olarak kaydedilir.

Daha sonra:

```text
currentTime - startTime
```

hesaplanarak geçen süre bulunur.

Geçen süre `intervalTime` değerine ulaştığında timer'ın süresi dolmuş kabul edilir.

---

# 📦 STimer_t Yapısı

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

timer'ı 500 ms için başlatır.

---

# 🔧 Driver Fonksiyonları

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

Temel olarak:

```text
startTime    → Mevcut msTick
intervalTime → İstenen süre
activated    → true
```

şeklinde ayarlanır.

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

Zaman bilgisini driver içerisinden almak için kullanılır.

---

## `Software_Timer_Clock_Check_Elapsed_Time()`

```c
bool Software_Timer_Clock_Check_Elapsed_Time(
    STimer_t *STimer
);
```

Software Timer'ın süresinin dolup dolmadığını kontrol eder.

Süre dolmadıysa:

```c
false
```

döndürür.

Süre dolduysa:

```c
true
```

döndürür ve timer'ı pasif hale getirir.

Periyodik kullanım için timer tekrar kurulmalıdır:

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

Bu işlem Hardware Timer'ı durdurmaz; yalnızca ilgili Software Timer'ı devre dışı bırakır.

---

# 🔢 Birden Fazla Software Timer

Tek bir Hardware Timer üzerinden birden fazla bağımsız Software Timer oluşturulabilir.

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

        Software_Timer_Set_Time(&timer_1s, 1000);
    }

    if (Software_Timer_Clock_Check_Elapsed_Time(&timer_500ms))
    {
        // 500 ms'lik görev

        Software_Timer_Set_Time(&timer_500ms, 500);
    }
}
```

şeklinde kullanılabilir.

Her Software Timer aynı `msTick` zaman tabanını kullanır ancak kendi:

```text
startTime
intervalTime
activated
```

bilgilerine sahiptir.

---

# 🔄 Overflow Koruması

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

Timer hesabında overflow durumu dikkate alınır.

Normal durumda:

```text
currentTime >= startTime
```

olurken, overflow sonrasında:

```text
currentTime < startTime
```

olabilir.

Driver bu durumu dikkate alarak geçen sürenin doğru şekilde hesaplanmasını sağlar.

---

# 💡 Neden Software Timer?

## `HAL_Delay()` yerine

`HAL_Delay()` kullanıldığında kod belirli bir süre boyunca bekler:

```c
HAL_Delay(1000);
```

Bu sırada ilgili kod akışında başka işlemler gerçekleştirilemez.

Software Timer ile:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // Görev
}
```

şeklinde kontrol yapılır.

Böylece uygulama diğer işlemlerini çalıştırmaya devam edebilir.

## Birden fazla zamanlayıcı

Tek bir Hardware Timer'ın oluşturduğu `msTick` zaman tabanı kullanılarak:

```text
500 ms Timer
1 s Timer
2 s Timer
5 s Timer
```

gibi birden fazla Software Timer oluşturulabilir.

Bu yaklaşım Hardware Timer kaynaklarının daha verimli kullanılmasını sağlar.

---

# 🛠️ Kodu Tekrar Yazmak İçin

Bu driver'ı ileride sıfırdan yazmak istersen temel mantığı şu şekilde hatırlayabilirsin:

### 1. Hardware Timer'ı düzenli interrupt verecek şekilde ayarla

```text
TIM11 → 1 ms
```

### 2. Global zaman sayacı oluştur

```c
uint32_t msTick = 0;
```

### 3. Her interrupt'ta artır

```c
msTick++;
```

### 4. Timer bilgilerini bir struct içerisinde tut

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

### 5. Timer başlatılırken zamanı kaydet

```text
startTime = msTick
```

### 6. Geçen zamanı hesapla

```text
currentTime - startTime
```

değerini `intervalTime` ile karşılaştır.

### 7. Süre dolduğunda timer'ı pasifleştir

```text
Süre doldu
    ↓
activated = false
    ↓
true
```

### 8. Periyodik görev gerekiyorsa tekrar başlat

```c
Software_Timer_Set_Time(&timer, 1000);
```

Özet:

```text
Hardware Timer
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

# ⚠️ Dikkat Edilmesi Gerekenler

- Hardware Timer interrupt'ı CubeMX üzerinden aktif edilmelidir.
- Timer periyodu driver'ın kullandığı zaman tabanına uygun olmalıdır.
- `software_Timer_Init()` çağrılmadan Software Timer kullanılmamalıdır.
- Periyodik timer kullanılıyorsa süre dolduktan sonra `Software_Timer_Set_Time()` tekrar çağrılmalıdır.
- `Software_Timer_Disable()` yalnızca ilgili Software Timer'ı durdurur.
- Farklı bir Hardware Timer kullanacaksanız ilgili Timer handle'ını driver'a vermeli ve callback içerisindeki Timer kontrolünü buna göre değiştirmelisiniz.

---

# 🔮 Geliştirilebilecek Kısımlar

Driver temel bir Software Timer altyapısı olarak tasarlanmıştır.

İleride:

- Otomatik periyodik timer modu
- Callback desteği
- Mikro saniye çözünürlüğü
- Tek seferlik / periyodik timer seçimi
- Daha gelişmiş timer yönetimi

gibi özellikler eklenebilir.

---

# 📁 Proje Yapısı

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

Software Timer veri yapısını ve driver fonksiyonlarının prototiplerini içerir.

### `Software_Timer.c`

Software Timer'ın çalışma mantığını, zaman kontrolünü ve overflow kontrolünü içerir.

### `main.c`

Driver'ın STM32 uygulaması içerisinde nasıl kullanılabileceğini gösterir.

---

# 🎯 Sonuç

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
