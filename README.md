# STM32F4 Software Timer Driver

STM32F4 mikrodenetleyicilerinde Donanımsal Timer (Hardware Timer - Örn: TIM11) kesmesini kullanarak **non-blocking (engellemesiz)**, çoklu yazılımsal zamanlayıcılar (Software Timers) oluşturulmasını sağlayan modüler bir sürücüdür. 

Bu driver sayesinde `HAL_Delay()` gibi CPU'yu kilitleyen gecikmeler yerine, tıpkı Arduino'daki `millis()` mantığına benzer şekilde birden fazla görevi milisaniye hassasiyetinde ve zaman aşımı (overflow) korumalı olarak paralel çalıştırabilirsiniz.

## 📋 İçindekiler
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

---

## 🚀 1. Hızlı Başlangıç

Driver'ı kendi STM32CubeIDE projenize dahil etmek için temel adımlar:

```text
Software_Timer.c / Software_Timer.h
        ↓
STM32CubeMX TIM11 Kesme (NVIC) Ayarları
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
