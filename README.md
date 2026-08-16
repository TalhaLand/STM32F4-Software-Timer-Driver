# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde bir Hardware Timer interrupt'ı kullanarak **non-blocking Software Timer** oluşturmayı sağlayan basit ve modüler bir C sürücüsüdür.

Driver, örnekte **TIM11** üzerinden 1 ms'lik bir zaman tabanı oluşturur. Bu zaman tabanı kullanılarak aynı Hardware Timer üzerinden birden fazla bağımsız Software Timer çalıştırılabilir.

## Özellikler

- Non-blocking Software Timer
- Hardware Timer interrupt tabanlı çalışma
- Milisaniye çözünürlüğünde zamanlama
- Tek Hardware Timer ile birden fazla Software Timer
- `uint32_t` overflow kontrolü
- Basit ve modüler API
- STM32 HAL ile uyumlu yapı

---

# 🚀 Hızlı Başlangıç

Driver'ı kendi STM32CubeIDE projenize eklemek için aşağıdaki adımlar yeterlidir.

## 1. Driver dosyalarını ekleyin

Projeye:

```text
Software_Timer.c
Software_Timer.h
```

dosyalarını ekleyin.

Kullanacağınız `.c` dosyasına:

```c
#include "Software_Timer.h"
```

ekleyin.

---

## 2. TIM11'i yapılandırın

Bu örnekte Software Timer için **TIM11** kullanılmaktadır.

CubeMX'te:

```text
TIM11 → Activated
TIM11 Global Interrupt → Enabled
```

olmalıdır.

Timer'ın **1 ms'de bir interrupt** üretmesi gerekir.

Bu projedeki örnekte:

```text
Timer Clock = 50 MHz
PSC = 49
ARR = 999
```

kullanılmıştır.

Hesap:

```text
50,000,000 / ((49 + 1) × (999 + 1))
= 1000 Hz
```

Dolayısıyla:

```text
1 / 1000 = 1 ms
```

---

## 3. Timer'ı başlatın

`main.c` içerisinde:

```c
software_Timer_Init(&htim11);
```

çağrılır.

---

## 4. Software Timer oluşturun

```c
STimer_t timer;
```

Timer'a süre verin:

```c
Software_Timer_Set_Time(&timer, 1000);
```

Burada `1000` milisaniyedir:

```text
1000 ms = 1 saniye
```

---

## 5. Timer'ı kontrol edin

```c
while (1)
{
    if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
    {
        // 1 saniye geçti

        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

        Software_Timer_Set_Time(&timer, 1000);
    }
}
```

Artık LED her 1 saniyede bir toggle edilebilir.

---

# 🧠 Driver Nasıl Çalışıyor?

Driver'ın temelinde TIM11 tarafından oluşturulan **1 ms'lik zaman tabanı** bulunur.

Her timer interrupt'ında:

```c
msTick++;
```

yapılır.

Örneğin:

```text
1 ms     → msTick = 1
10 ms    → msTick = 10
100 ms   → msTick = 100
1000 ms  → msTick = 1000
```

Software Timer ise bu değeri kullanarak geçen zamanı hesaplar.

Temel mantık:

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
Süre dolduysa → true
```

Bu sayede `HAL_Delay()` kullanmadan zaman kontrollü işlemler yapılabilir.

---

# ⚡ Interrupt ve `msTick`

Driver'ın zaman tabanı:

```c
uint32_t msTick = 0;
```

değişkenidir.

TIM11 interrupt'ı geldiğinde callback içerisinde:

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

Bu nedenle **TIM11 interrupt'ının aktif olması driver için zorunludur.**

---

# 📦 `STimer_t` Yapısı

Her Software Timer için aşağıdaki yapı kullanılır:

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

Alanların görevleri:

| Alan | Açıklama |
|---|---|
| `startTime` | Timer'ın başlatıldığı andaki `msTick` değeri |
| `intervalTime` | Beklenecek süre, ms cinsinden |
| `activated` | Timer'ın aktif olup olmadığını belirtir |

Örneğin:

```c
STimer_t timer;

Software_Timer_Set_Time(&timer, 500);
```

çağrıldığında timer'ın başlangıç zamanı kaydedilir ve `500 ms` beklemesi gerektiği belirtilir.

---

# 🔧 Driver Fonksiyonları

## `software_Timer_Init()`

```c
void software_Timer_Init(TIM_HandleTypeDef *htim);
```

Hardware Timer'ı interrupt modunda başlatır.

Temel olarak:

```c
HAL_TIM_Base_Start_IT(htim);
```

kullanılır.

Kullanım:

```c
software_Timer_Init(&htim11);
```

Bu fonksiyon **Software Timer'ı değil, Hardware Timer'ı başlatır.**

---

## `Software_Timer_Set_Time()`

```c
void Software_Timer_Set_Time(
    STimer_t *STimer,
    uint32_t intervalMs
);
```

Software Timer'ı başlatır veya yeniden kurar.

Örneğin:

```c
Software_Timer_Set_Time(&timer, 1000);
```

çağrıldığında:

```text
startTime    → mevcut msTick
intervalTime → 1000 ms
activated    → true
```

olarak ayarlanır.

Periyodik bir timer kullanmak istiyorsanız süre dolduktan sonra bu fonksiyonu tekrar çağırabilirsiniz.

---

## `Software_Timer_Get_Time()`

```c
uint32_t Software_Timer_Get_Time(void);
```

Mevcut `msTick` değerini döndürür.

Driver içerisinde zaman bilgisini almak için kullanılır.

---

## `Software_Timer_Clock_Check_Elapsed_Time()`

```c
bool Software_Timer_Clock_Check_Elapsed_Time(
    STimer_t *STimer
);
```

Timer'ın süresinin dolup dolmadığını kontrol eder.

Süre dolduysa:

```c
true
```

döndürür.

Süre dolmadıysa:

```c
false
```

döndürür.

Örnek:

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // Süre doldu
}
```

Bu fonksiyon `while(1)` içerisinde tekrar tekrar çağrılabilir.

---

## `Software_Timer_Disable()`

```c
void Software_Timer_Disable(STimer_t *STimer);
```

İlgili Software Timer'ı pasif hale getirir.

```c
Software_Timer_Disable(&timer);
```

Bu işlem Hardware Timer'ı durdurmaz. Sadece ilgili Software Timer'ın çalışmasını durdurur.

---

# 🔢 Birden Fazla Software Timer

Tek bir Hardware Timer kullanılarak birden fazla Software Timer oluşturulabilir.

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

Burada iki timer da aynı `msTick` değerini kullanır:

```text
             TIM11
               │
               ▼
             msTick
            /      \
           /        \
          ▼          ▼
      Timer 1s    Timer 500ms
```

Her timer kendi `startTime` ve `intervalTime` değerine sahip olduğu için birbirinden bağımsız çalışır.

---

# 🔄 `uint32_t` Overflow

`msTick` değişkeni `uint32_t` olduğu için maksimum değeri:

```text
4,294,967,295
```

olabilir.

1 ms çözünürlükte yaklaşık **49.7 gün** sonra değer tekrar `0`'a döner.

```text
4,294,967,295
        ↓
        0
        ↓
        1
        ↓
        2
```

Driver'daki elapsed-time kontrolü bu overflow durumunu dikkate alacak şekilde hazırlanmıştır.

Böylece sistem uzun süre çalıştığında `msTick` değerinin sıfırlanması Software Timer'ın yanlış çalışmasına neden olmaz.

---

# 🧪 Örnek Kullanım

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

Burada:

```text
TIM11 → 1 ms zaman tabanı
       ↓
    msTick
       ↓
  ledTimer
       ↓
   1000 ms
       ↓
    LED Toggle
```

şeklinde çalışır.

---

# 🛠️ Kodun Temel Mantığını Tekrar Yazmak İstersen

Bu driver'ı ileride sıfırdan yazmak istersen temel olarak şu yapıyı hatırlaman yeterlidir:

### 1. Hardware Timer'dan düzenli interrupt üret

```text
TIM11 → 1 ms
```

### 2. Her interrupt'ta zamanı artır

```c
msTick++;
```

### 3. Timer bilgilerini bir struct içerisinde tut

```c
typedef struct
{
    uint32_t startTime;
    uint32_t intervalTime;
    bool activated;

} STimer_t;
```

### 4. Timer başlatılırken başlangıç zamanını kaydet

```text
startTime = msTick
```

### 5. Geçen zamanı kontrol et

```text
currentTime - startTime
```

değerini istenen süreyle karşılaştır.

### 6. Süre dolduysa görevi çalıştır

```c
if (Software_Timer_Clock_Check_Elapsed_Time(&timer))
{
    // Görev
}
```

### 7. Periyodik çalışacaksa timer'ı tekrar kur

```c
Software_Timer_Set_Time(&timer, 1000);
```

Temel fikir bundan ibarettir:

```text
Hardware Timer
      ↓
   msTick
      ↓
Software Timer
      ↓
Elapsed Time
      ↓
    Task
```

---

# ⚠️ Dikkat Edilmesi Gerekenler

- TIM11 interrupt'ı CubeMX'te aktif olmalıdır.
- Timer'ın periyodu driver'ın kullandığı zaman tabanına uygun olmalıdır.
- `software_Timer_Init()` çağrılmadan Software Timer kullanılmamalıdır.
- Periyodik timer kullanıyorsanız süre dolduktan sonra `Software_Timer_Set_Time()` tekrar çağrılmalıdır.
- `Software_Timer_Disable()` yalnızca ilgili Software Timer'ı durdurur; Hardware Timer çalışmaya devam eder.

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

Driver'ın veri yapılarını ve fonksiyon prototiplerini içerir.

### `Software_Timer.c`

Software Timer'ın çalışma mantığını ve zaman kontrol fonksiyonlarını içerir.

### `main.c`

Driver'ın STM32 uygulaması içerisinde nasıl kullanılabileceğini gösterir.

---

# 🎯 Sonuç

Bu driver, STM32F4 üzerinde tek bir Hardware Timer kullanarak birden fazla **non-blocking Software Timer** oluşturmayı sağlar.

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

Bu yapı sayesinde `HAL_Delay()` kullanmadan farklı görevleri belirli zaman aralıklarında çalıştırmak mümkün olur.

---

## 👨‍💻 Author

**Talha Mansur Şahin**

STM32 | Embedded Systems | C
