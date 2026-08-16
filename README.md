# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde Donanımsal Timer (Hardware Timer - Örn: TIM11) kesmesini kullanarak **non-blocking (engellemesiz)**, çoklu yazılımsal zamanlayıcılar (Software Timers) oluşturulmasını sağlayan modüler bir sürücüdür.

Bu driver sayesinde HAL_Delay() gibi CPU'yu kilitleyen gecikmeler yerine, tıpkı Arduino'daki millis() mantığına benzer şekilde birden fazla görevi milisaniye hassasiyetinde ve zaman aşımı (overflow) korumalı olarak paralel çalıştırabilirsiniz.

## İçindekiler
- [Özellikler](#özellikler)
- [1. Hızlı Başlangıç](#1-hızlı-başlangıç)
- [2. Driver Dosyalarını Projeye Ekleme](#2-driver-dosyalarını-projeye-ekleme)
- [3. STM32CubeMX Ayarları](#3-stm32cubemx-ayarları)
- [4. Clock Tree ve Prescaler / Period Hesabı](#4-clock-tree-ve-prescaler--period-hesabı)
- [5. Driver'ı Başlatma](#5-driverı-başlatma)
- [6. main.c Kullanımı](#6-mainc-kullanımı)
- [7. Driver'ın Çalışma Mantığı](#7-driverın-çalışma-mantığı)
- [8. Kesme (Interrupt) ve Tick Akışı](#8-kesme-interrupt-ve-tick-akışı)
- [9. Overflow (Taşma) Koruması](#9-overflow-taşma-koruması)
- [10. STimer_t Yapısı](#10-stimer_t-yapısı)
- [11. Driver Fonksiyonları](#11-driver-fonksiyonları)
- [12. Tam Kullanım Örneği](#12-tam-kullanım-örneği)
- [13. Neden Bu Mimariyi Kullandık?](#13-neden-bu-mimariyi-kullandık)
- [14. Geliştirilebilecek Kısımlar](#14-geliştirilebilecek-kısımlar)
- [15. Proje Yapısı](#15-proje-yapısı)
- [16. 6 Ay Sonra Hatırlamak İçin](#16-6-ay-sonra-hatırlamak-için)
- [17. Sonuç](#17-sonuç)

---

## Özellikler
* Non-blocking (engellemesiz) zamanlama
* Tek bir Hardware Timer ile sınırsız sayıda Software Timer oluşturabilme
* uint32_t overflow (taşma) koruması (49.7 gün sınırı koruması)
* Milisaniye (ms) hassasiyetinde zamanlama
* Modüler ve taşınabilir C kütüphanesi
* Kolay kullanım sağlayan STimer_t yapısı

---

## 1. Hızlı Başlangıç

Driver'ı kendi STM32CubeIDE projenize dahil etmek için temel olarak:

Software_Timer.c / Software_Timer.h
        ↓
STM32CubeMX TIM11 Interrupt Ayarları
        ↓
#include "Software_Timer.h"
        ↓
STimer_t stimer_1s;
        ↓
software_Timer_Init(&htim11);
        ↓
Software_Timer_Set_Time(&stimer_1s, 1000);
        ↓
while(1) { Check_Elapsed_Time(...) }

şeklinde ilerlenir.

---

## 2. Driver Dosyalarını Projeye Ekleme

Repository içerisindeki Software_Timer.c ve Software_Timer.h dosyalarını projenize ekleyin.

Örneğin:

MyProject/
│
├── Core/
│   ├── Inc/
│   │   └── Software_Timer.h
│   └── Src/
│       └── Software_Timer.c
└── ...

Daha sonra driver'ı kullanacağınız .c dosyasına ekleyin:

#include "Software_Timer.h"

---

## 3. STM32CubeMX Ayarları

Driver'ın doğru çalışabilmesi için 1 ms periyotla kesme üreten bir Hardware Timer yapılandırılmalıdır.

Timer Ayarları (TIM11 Örneği):
* Activated: Enabled
* NVIC Settings: TIM11 global interrupt ➔ Enabled

Önemli: CubeMX üzerinde NVIC sekmesinden Timer kesmesinin (Interrupt) etkinleştirildiğinden emin olunmalıdır. Unutulursa msTick değişkeni artmaz!

---

## 4. Clock Tree ve Prescaler / Period Hesabı

Timer'ın tam 1 ms (1000 Hz) periyotla kesme üretmesi için Clock yapılandırması:

* SYSCLK / APB2 Timer Clock: 50 MHz
* Prescaler (PSC): 49
* Counter Period (ARR): 999

Hesaplama Formülü:
F_interrupt = F_timer / ((PSC + 1) * (ARR + 1))
F_interrupt = 50.000.000 / ((49 + 1) * (999 + 1)) = 1000 Hz -> 1 ms

---

## 5. Driver'ı Başlatma

Öncelikle bir timer yapısı oluşturun:

STimer_t stimer_1s;

Daha sonra CubeMX tarafından oluşturulan Timer handle'ını driver'a gönderin ve süreyi kurun:

software_Timer_Init(&htim11);
Software_Timer_Set_Time(&stimer_1s, 1000); // Ms cinsinden değer (1000 ms = 1 sn)

---

## 6. main.c Kullanımı

Temel kullanım:

#include "main.h"
#include "Software_Timer.h"

TIM_HandleTypeDef htim11;
STimer_t stimer_1s;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM11_Init();

    software_Timer_Init(&htim11);
    Software_Timer_Set_Time(&stimer_1s, 1000);

    while (1)
    {
        if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_1s))
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            Software_Timer_Set_Time(&stimer_1s, 1000);
        }
    }
}

---

## 7. Driver'ın Çalışma Mantığı

Driver'ın temel veri akışı:

            Hardware Timer (TIM11)
                      │ (1 ms Interrupt)
                      ▼
       HAL_TIM_PeriodElapsedCallback()
                      │
                      ▼
                   msTick++
                      │
                      ▼
   Software_Timer_Clock_Check_Elapsed_Time()
                      │
                      ▼
     Geçen Süre >= intervalTime Yapıldı mı?
                      │
            ┌─────────┴─────────┐
            ▼                   ▼
         [Evet]              [Hayır]
            │                   │
            ▼                   ▼
     Software_Timer_Disable()  Beklemeye Devam Et
            │
            ▼
        Return true

Driver'ın amacı, donanımsal timer sayacını yazılımsal tick mantığıyla birleştirerek CPU'yu engellemeden zaman takibi yapmaktır.

---

## 8. Kesme (Interrupt) ve Tick Akışı

Hardware Timer her 1 ms'de bir periyot tamamlayarak kesmeye girer ve callback fonksiyonunu çalıştırır:

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == TIM11)
    {
        // 1 ms kesme çalıştı demektir
        msTick++;
    }
}

Bu sayede sistem çalıştığı sürece msTick değişkeni her milisaniyede 1 artarak zaman referansını oluşturur.

---

## 9. Overflow (Taşma) Koruması

uint32_t türündeki msTick değişkeni 4.294.967.295 değerine ulaştığında (yaklaşık 49.7 gün sonra) 0'a taşar.

Driver içerisinde yazılan algoritmik kontrol sayesinde taşma durumu yönetilir:

if(STimer->startTime <= currentTick)
{
    if(currentTick - STimer->startTime >= STimer->intervalTime)
    {
        Software_Timer_Disable(STimer);
        return true;
    }
}
else
{
    // Overflow durumu (currentTick sıfırlandı)
    if((0xFFFFFFFF - (STimer->startTime - currentTick)) >= STimer->intervalTime)
    {
        Software_Timer_Disable(STimer);
        return true;
    }
}

Bu sayede cihaz 50 günden fazla kesintisiz çalışsa dahi zamanlama mantığı bozulmaz.

---

## 10. STimer_t Yapısı

Driver'ın tüm zamanlayıcı bilgileri STimer_t yapısı içerisinde tutulur:

typedef struct
{
    uint32_t startTime;    // Timer'ın kurulduğu andaki msTick değeri
    uint32_t intervalTime; // Çalışması istenen hedef süre (ms)
    bool     activated;    // Timer'ın aktiflik durumu
} STimer_t;

---

## 11. Driver Fonksiyonları

### software_Timer_Init()
Hardware Timer kesmesini başlatır.

void software_Timer_Init(TIM_HandleTypeDef *htim)
{
    HAL_TIM_Base_Start_IT(htim);
}

### Software_Timer_Set_Time()
Zamanlayıcıya süre yükler, başlangıç zamanını kaydeder ve aktif eder.

void Software_Timer_Set_Time(STimer_t *STimer, uint32_t intervalMs)
{
    STimer->startTime    = Software_Timer_Get_Time();
    STimer->intervalTime = intervalMs;
    STimer->activated    = true;
}

### Software_Timer_Get_Time()
Güncel msTick değerini döndürür.

uint32_t Software_Timer_Get_Time(void)
{
    return msTick;
}

### Software_Timer_Clock_Check_Elapsed_Time()
Sürenin dolup dolmadığını kontrol eder. Süre dolduysa zamanlayıcıyı durdurur ve true döner.

bool Software_Timer_Clock_Check_Elapsed_Time(STimer_t *STimer);

### Software_Timer_Disable()
Zamanlayıcıyı pasif hale getirir.

void Software_Timer_Disable(STimer_t *STimer)
{
    STimer->activated = false;
}

---

## 12. Tam Kullanım Örneği

Aşağıda birden fazla zamanlayıcının aynı anda non-blocking olarak nasıl çalıştırıldığı gösterilmiştir:

#include "main.h"
#include "Software_Timer.h"

TIM_HandleTypeDef htim11;

STimer_t stimer_1s;
STimer_t stimer_500ms;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM11_Init();

    software_Timer_Init(&htim11);

    Software_Timer_Set_Time(&stimer_1s, 1000);
    Software_Timer_Set_Time(&stimer_500ms, 500);

    while (1)
    {
        if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_1s))
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            Software_Timer_Set_Time(&stimer_1s, 1000);
        }

        if(Software_Timer_Clock_Check_Elapsed_Time(&stimer_500ms))
        {
            // Farklı bir işlem yap
            Software_Timer_Set_Time(&stimer_500ms, 500);
        }
    }
}

---

## 13. Neden Bu Mimariyi Kullandık?

* CPU Verimliliği: HAL_Delay() fonksiyonu CPU'yu boş döngüde kilitler. Bu mimaride CPU diğer işlemleri yapmaya devam edebilir.
* Modülerlik: Tek bir donanımsal zamanlayıcı kaynağı kullanılarak istenildiği kadar yazılımsal zamanlayıcı oluşturulabilir.
* Donanım Soyutlama: Uygulama katmanı timer register'ları ile ilgilenmez, sadece süre tanımlar ve kontrol eder.

---

## 14. Geliştirilebilecek Kısımlar

* Auto-reload Desteği: Süre dolduğunda kendiliğinden tekrar kurulan periyodik timer modu eklenebilir.
* Microsecond (us) Desteği: Mikrosaniye seviyesinde hassas zamanlama için tick çözünürlüğü artırılabilir.
* Callback Desteği: Süre dolduğunda doğrudan çağrılacak function pointer (callback) yapısı eklenebilir.

---

## 15. Proje Yapısı

Önerilen yapı:

STM32F4-Software-Timer-Driver/
│
├── Software_Timer.c
├── Software_Timer.h
├── main.c
├── main.h
└── README.md

---

## 16. 6 Ay Sonra Hatırlamak İçin

Bu driver'ın bütün mantığını tekrar hatırlamak için şu sırayı düşün:

1. Hardware Timer (TIM11) 1 ms'de bir kesmeye girer.
2. Kesme içinde msTick değişkeni artırılır.
3. Software_Timer_Set_Time() ile başlangıç zamanı (startTime) ve süre (intervalTime) kaydedilir.
4. Software_Timer_Clock_Check_Elapsed_Time() fonksiyonu currentTick - startTime farkını alır.
5. Fark hedeflenen intervalTime süresine ulaştıysa timer durdurulur ve true döner.
6. while(1) içinde bu true bilgisi yakalanıp ilgili görev çalıştırılır ve timer isteğe bağlı yeniden kurulur.

Kısacası:
Hardware Timer (1ms) → Interrupt → msTick++ → Set_Time → Check_Elapsed_Time → Application

---

## 17. Sonuç

Bu driver, STM32F4 üzerinde donanımsal kesme altyapısını kullanarak karmaşık zamanlama ihtiyaçlarını çözmek için geliştirilmiştir. Non-blocking yapısı ve overflow koruması sayesinde endüstriyel gömülü sistem uygulamalarında güvenle kullanılabilir.
