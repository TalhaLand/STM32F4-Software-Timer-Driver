# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde Donanımsal Timer (Hardware Timer - Örn: TIM11) kesmesini kullanarak non-blocking (engellemesiz), çoklu yazılımsal zamanlayıcılar (Software Timers) oluşturulmasını sağlayan modüler bir sürücüdür. Bu driver sayesinde `HAL_Delay()` gibi CPU'yu kilitleyen gecikmeler yerine, tıpkı Arduino'daki `millis()` mantığına benzer şekilde birden fazla görevi milisaniye hassasiyetinde ve zaman aşımı (overflow) korumalı olarak paralel çalıştırabilirsiniz.

## İçindekiler
1. Hızlı Başlangıç
2. Driver Dosyalarını Projeye Ekleme
3. STM32CubeMX Ayarları
4. Clock Tree ve Prescaler / Period Hesabı
5. Driver'ı Başlatma ve main.c Kullanımı
6. Driver'ın Çalışma Mantığı
7. Kesme (Interrupt) ve Tick Akışı
8. Overflow (Taşma) Koruması
9. STimer_t Yapısı
10. Driver Fonksiyonları
11. Tam Kullanım Örneği
12. Neden Bu Mimariyi Kullandık?
13. 6 Ay Sonra Hatırlamak İçin

## 🚀 1. Hızlı Başlangıç
Driver'ı kendi STM32CubeIDE projenize dahil etmek için temel adımlar:
Software_Timer.c / Software_Timer.h → STM32CubeMX TIM11 Kesme (NVIC) Ayarları → #include "Software_Timer.h" → STimer_t stimer_1s; (Nesne Oluştur) → software_Timer_Init(&htim11); → Software_Timer_Set_Time(&stimer_1s, 1000); → while(1) { Check_Elapsed_Time(...) }

## 📁 2. Driver Dosyalarını Projeye Ekleme
Repository içerisindeki `Software_Timer.c` ve `Software_Timer.h` dosyasını projenize ekleyin:
MyProject/
├── Core/
│   ├── Inc/
│   │   └── Software_Timer.h
│   └── Src/
│       └── Software_Timer.c

Kullanacağınız `.c` dosyasına ekleyin:
#include "Software_Timer.h"

## ⚙️ 3. STM32CubeMX Ayarları
Driver, milisaniye cinsinden zaman sayabilmek için 1 ms'de bir kesme (interrupt) üreten bir Timer birimine ihtiyaç duyar.
TIM11 için örnek yapılandırma:
- Activated: Enabled
- NVIC Settings: TIM11 global interrupt → Enabled (Unutulursa kesmeye girmez!)

## 🕐 4. Clock Tree ve Prescaler / Period Hesabı
Timer'ın tam 1 ms (1000 Hz) periyotla kesme üretmesi için Clock ayarları doğru yapılmalıdır.
- SYSCLK / APB2 Timer Clock: 50 MHz
- Prescaler (PSC): 49 (50 MHz / (49 + 1) = 1 MHz = 1.000.000 Hz)
- Counter Period (ARR): 999 (1 MHz / (999 + 1) = 1.000 Hz = 1 ms)
Formül: F_interrupt = F_timer / ((PSC + 1) * (ARR + 1)) ➔ 50.000.000 / (50 * 1000) = 1000 Hz = 1 ms

## 🚀 5. Driver'ı Başlatma ve main.c Kullanımı
1. Zamanlayıcı Değişkenini Tanımlayın:
STimer_t stimer_1s;

2. Başlatın ve Süreyi Kurun (main fonksiyonunda):
software_Timer_Init(&htim11);
Software_Timer_Set_Time(&stimer_1s, 1000); // 1000 ms (1 saniye)

3. while(1) Döngüsünde Kontrol Edin:
if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_1s))
{
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    Software_Timer_Set_Time(&stimer_1s, 1000);
}

## 🧠 6. Driver'ın Çalışma Mantığı
Driver, Hardware Timer Interrupt ile Software Tick mantığını birleştirir:
Hardware Timer (TIM11 - 1ms) → HAL_TIM_PeriodElapsedCallback() → msTick++ (Sürekli 1 artar) → Software_Timer_Clock_Check_Elapsed_Time() → (currentTick - startTime >= intervalTime) → EVET ➔ Timer bitti! (true döner) / HAYIR ➔ Beklemeye devam et (false döner)

## 🔄 7. Kesme (Interrupt) ve Tick Akışı
Her 1 ms'de bir donanım kesmesi tetiklenir ve HAL_TIM_PeriodElapsedCallback fonksiyonu çağrılarak yazılımsal sayaç artırılır:
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM11)
    {
        msTick++;
    }
}

## 🛡️ 8. Overflow (Taşma) Koruması
msTick değişkeni uint32_t türündedir ve yaklaşık 49.7 gün sonra maksimum değerine (0xFFFFFFFF) ulaşarak 0'a taşar. Driver bu durumu engellemek için özel bir kontrol algoritmasına sahiptir:
if(STimer->startTime <= currentTick)
{
    if(currentTick - STimer->startTime >= STimer->intervalTime) { ... }
}
else
{
    if((0xFFFFFFFF - (STimer->startTime - currentTick)) >= STimer->intervalTime) { ... }
}
Bu sayede sistem 50 gün aralıksız çalışsa bile zamanlayıcılar hatalı tetiklenmez!

## 📦 9. STimer_t Yapısı
Her bir yazılımsal zamanlayıcı aşağıdaki veri yapısını kullanır:
typedef struct
{
    uint32_t startTime;    // Timer'ın başlatıldığı andaki msTick değeri
    uint32_t intervalTime; // Hedeflenen bekleme süresi (ms)
    bool     activated;    // Timer aktif mi / pasif mi bilgisi
} STimer_t;

## 🔧 10. Driver Fonksiyonları
- software_Timer_Init(TIM_HandleTypeDef *htim): Seçilen Hardware Timer kesmesini başlatır (HAL_TIM_Base_Start_IT).
- Software_Timer_Set_Time(STimer_t *STimer, uint32_t intervalMs): Zamanlayıcıya hedef süre yükler, başlangıç zamanını kaydeder ve aktif eder.
- Software_Timer_Get_Time(void): Güncel msTick değerini döndürür.
- Software_Timer_Clock_Check_Elapsed_Time(STimer_t *STimer): Sürenin dolup dolmadığını kontrol eder, dolduysa zamanlayıcıyı durdurup true döndürür.
- Software_Timer_Disable(STimer_t *STimer): Zamanlayıcıyı pasife çeker (activated = false).

## 🧩 11. Tam Kullanım Örneği
#include "main.h"
#include "Software_Timer.h"

TIM_HandleTypeDef htim11;
STimer_t stimer_led1;
STimer_t stimer_led2;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM11_Init();

    software_Timer_Init(&htim11);

    Software_Timer_Set_Time(&stimer_led1, 500);  // 500 ms
    Software_Timer_Set_Time(&stimer_led2, 1000); // 1000 ms

    while (1)
    {
        if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_led1))
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            Software_Timer_Set_Time(&stimer_led1, 500);
        }

        if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_led2))
        {
            // Başka bir işlem yap
            Software_Timer_Set_Time(&stimer_led2, 1000);
        }
    }
}

## 🏗️ 12. Neden Bu Mimariyi Kullandık?
1. Non-blocking (Engellemesiz) Çalışma: HAL_Delay() gibi CPU'yu meşgul etmez. CPU döngüde diğer işleri yürütmeye devam edebilir.
2. Sınırsız Timer Oluşturma: Tek bir donanımsal Timer (TIM11) kullanarak yazılım tarafında dilediğiniz kadar (STimer_t) zamanlayıcı oluşturabilirsiniz.
3. Modülerlik: Uygulama katmanı donanım detaylarıyla ilgilenmez, sadece süre kurar ve kontrol eder.

## 🧠 13. 6 Ay Sonra Hatırlamak İçin
Bu driver'ın çalışma mantığını hızlıca hatırlamak için şu sırayı düşün:
1. TIM11 donanımı 1 ms'de bir kesmeye girer.
2. Kesme içinde msTick değişkeni sürekli 1 artar.
3. Software_Timer_Set_Time() çağrıldığında o anki msTick değeri startTime olarak kaydedilir.
4. while(1) içinde Check_Elapsed_Time() ile (O anki msTick - startTime) farkı hesaplanır.
5. Fark intervalTime değerine ulaştıysa timer biter (true döner) ve otomatik kendisini durdurur (activated = false).
6. Tekrar çalışması için Set_Time() ile yeniden kurulması gerekir.
