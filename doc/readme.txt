Board:
https://www.amazon.de/dp/B0DMNBWTFD?ref=ppx_yo2ov_dt_b_fed_asin_title

ESP32-C3 unterstützt 2,4 GHz Wi-Fi (802.11 b/g/n) und BT 5 (LE), verfügt über 400 KB integrierten SRAM und 384 KB ROM-Speicherplatz sowie 4 MB integrierten Flash-Speicher. 

Wie man in den Download-Modus gelangt ；Wie man sich mit einem Computer verbindet, wenn die Verbindung ständig unterbrochen ist Methode 1: Drücken und halten Sie die BOOT-Taste auf dem ESP32-C3 SUPERMINI, wenn er bereits mit dem USB verbunden ist. wenn er bereits mit USB verbunden ist, dann tippen Sie auf den RESET-Knopf, dann lassen Sie den BOOT-Knopf los Methode 2: Halten Sie den BOOT-Knopf gedrückt, greifen Sie auf den USB zu und lassen Sie den Knopf los. An diesem Punkt wird das Board in den Download-Modus wechseln. 





IO:

I²C:
  GPIO8  → SDA (INA219)
  GPIO9  → SCL (INA219)

Relais:
  GPIO6  → Relais IN1 (Laden)
  GPIO7  → Relais IN2 (Entladen)

Reserve:
  GPIO2  → ADC (Spannung später, unbeschaltet)

Optional:
  GPIO10 → Status-LED
  GPIO1  → Taster


Schaltung:

