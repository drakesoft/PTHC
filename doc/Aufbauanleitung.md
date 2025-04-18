
# Bestückungs- und Montageanleitung für "Esphome PTHC"

---

## ⚠️ Haftungsausschluss / Disclaimer

**Wichtiger Hinweis:**  
Dieser Bausatz richtet sich ausschließlich an erfahrene Anwender mit Kenntnissen in der Elektronikentwicklung.

Die nachfolgende Anleitung beschreibt eine beispielhafte Umsetzung zum Aufbau elektronischer Komponenten. Es handelt sich hierbei **nicht um ein fertiges Produkt**, sondern um eine technische Dokumentation zur **eigenverantwortlichen Realisierung**.

---

## ❗ Achtung: Arbeiten mit Netzspannung

Bei Schaltungen, die mit **Netzstrom** betrieben werden oder damit in Verbindung stehen, gelten folgende Sicherheitsvorkehrungen:

- Arbeiten an Netzstrom können zu **lebensgefährlichen Stromschlägen** führen.
- Unsachgemäßer Aufbau oder Verwendung kann **Verletzungen, Brände oder tödliche Unfälle** verursachen.
- Netzspannungsführende Komponenten dürfen **ausschließlich von qualifiziertem Fachpersonal** montiert und geprüft werden.
- Bei Unsicherheiten: **Nicht selbst durchführen**, sondern eine Elektrofachkraft konsultieren.

---

## ⚠️ Allgemeine Sicherheitshinweise

1. Die gelieferten Komponenten sind **kein geprüftes oder sicheres Endgerät**.
2. Der Zusammenbau erfolgt **ausschließlich auf eigene Verantwortung und Gefahr**.
3. Für Schäden, Fehlfunktionen oder Verletzungen, die aus dem Aufbau oder der Nutzung entstehen, wird **keinerlei Haftung übernommen**.
4. Die Anleitung dient **rein zu Demonstrations-, Lern- und Forschungszwecken**.
5. Eine Verwendung in sicherheitskritischen Bereichen (z. B. Medizintechnik, Luftfahrt, Fahrzeugbau) ist **ausdrücklich ausgeschlossen**.

---

## 🛡️ Rechtlicher Hinweis

Diese Schaltung stellt ein Bastelprojekt und einen sogenannten "Proof-of-Concept" dar. Sie wurde **nicht nach geltenden Normen geprüft oder zertifiziert**. Der Nachbau geschieht auf eigene Gefahr.  
Es erfolgt keine Lieferung eines fertigen Elektrogeräts. Für fehlerhafte Montage oder unsachgemäße Verwendung wird keine Haftung übernommen.

Mit der Nutzung oder dem Erwerb der Anleitung erkennen Sie diesen Haftungsausschluss ausdrücklich an.

---

## 🔧 Einleitung

Diese Anleitung begleitet dich Schritt für Schritt durch den Aufbau der Platine sowie die Endmontage des Gesamtsystems.  
Sie richtet sich insbesondere an ambitionierte Einsteiger und enthält hilfreiche Hinweise und Erläuterungen für einen erfolgreichen Aufbau.

---

## 🧰 Benötigte Werkzeuge

- Lötstation oder Lötkolben
- Lötzinn
- Seitenschneider
- Multimeter
- Drehrichtungsanzeiger (z. B. SM852B)

## 📦 Erforderliches Installationsmaterial

- PTHC-Bausatz inkl. optionaler Komponenten → erhältlich unter www.draketronic.de
- Kabelverschraubungen
- Geeignetes Kabelmaterial (siehe Tabelle unten)
- 3-phasiger Heizstab (Dreieckschaltung, max. Leistung beachten)
- 3-poliger Leitungsschutzschalter
- 3-poliger Fehlerstromschutzschalter
- ESP32-DevKitC (wahlweise ESP32-DevKitC-32U mit externer Antenne)

## Einleitung:

Diese Anleitung führt dich Schritt-für-Schritt durch die Bestückung und Montage der Leiterplatte sowie die Endmontage des Systems. Sie richtet sich insbesondere an Anfänger und enthält zusätzliche Erläuterungen zu den einzelnen Schritten.

## Benötigtes Werkzeug:

- Lötstation oder Lötkolben
- Lötzinn
- Seitenschneider
- Multimeter
- Drehrichtungsanzeiger z.B. SM852B

## Benötigtes Installationsmaterial:

- PTHC Bausatz und ggf. Optionen -> www.draketronic.de
- Kabelverschraubungen
- Entsprechendes Kabelmaterial (siehe Tabellen unten)
- 3-Phasiger Heizstab (max. Leistung in Dreieckschaltung)
- Leistungsschutzschalter 3-polig
- Fehlerstrom-Schutzschalter 3-polig 
- ESP32-DevKitC, nach Bedarf auch ESP32-DevKitC-32U mit externer Antenne

# Schritt-für-Schritt-Bestückung "PTHC_CTRL_BRD"

## Leiterplate Oberseite:

<img src="CTRL_BRD_TOP.png" alt="drawing top" width="1000"/>

## Leiterplate Unterseite:

<img src="CTRL_BRD_BOT.png" alt="drawing bot" width="1000"/>


### Schritt 1: Widerstände bestücken
Widerstände sind nicht gepolt und können in beliebiger Richtung eingebaut werden. Die Werte werden häufig durch farbige Ringe angegeben. Alternativ kannst du den Widerstandswert mit einem Multimeter prüfen.

| Bauteil | Wert | Typ        | Farbringe                       |
|---------|------|------------|---------------------------------|
| R1      | 33Ω  | Widerstand | Orange - Orange - Schwarz - Gold|
| R3      | 51kΩ | Widerstand | Grün - Braun - Orange - Gold    |
| R5      | 440Ω | Widerstand | Gelb - Gelb - Braun - Gold      |
| R6      | 10kΩ | Widerstand | Braun - Schwarz - Orange - Gold |

Setze die Widerstände entsprechend der Markierung (R1, R3, R5, R6) auf der Platine ein, löte sie fest und kürze anschließend die überstehenden Beinchen mit dem Seitenschneider.

Falls du unsicher bist, nutze ein Multimeter, um den Widerstandswert eindeutig zu bestimmen.

### Schritt 2: Varistor bestücken
Ein Varistor schützt die Schaltung vor Überspannungen.

| Bauteil | Wert    | Typ      |
|---------|---------|----------|
| R2      | 10D561K | Varistor |

Setze den Varistor auf die Position R2. Auch dieser ist nicht gepolt und kann beliebig eingebaut werden.

### Schritt 3: Kondensatoren bestücken
Achtung: Elektrolyt-Kondensatoren (polarisiert) haben eine Polarität. Achte auf die Kennzeichnung (Plus/Minus).

| Bauteil | Wert  | Polarität |
|---------|-------|-----------|
| C1      | 330µF | Ja        |
| C2      | 100nF | Nein      |
| C3      | 220µF | Ja        |
| C4      | 1µF   | Nein      |
| C5      | 1µF   | Nein      |
| C6      | 330µF | Ja        |

Bestücke zuerst die nicht polarisierten Kondensatoren (C2, C4, C5), danach die polarisierten Kondensatoren (C1, C3, C6). Achte auf die markierte Polarität (Minus-Strich auf Kondensatorgehäuse).

### Schritt 4: Dioden bestücken
Dioden sind gepolt. Achte auf den Ring an der Diode, der mit der Markierung auf der Platine übereinstimmen muss.

| Bauteil | Wert     | Typ           |
|---------|----------|---------------|
| D1      | SR160    | Diode         |
| D2      | BZX85C43 | Z-Diode       |

### Schritt 5: Induktivitäten bestücken
Induktivitäten (Spulen) sind in diesem Fall nicht gepolt.

| Bauteil | Wert               | Typ           |
|---------|--------------------|---------------|
| L1      | PDUUAT98-103MLN    | Drossel       |
| L2      | PDSH0512I-680M     | Induktivität  |
| L4      | PDSH0512I-680M     | Induktivität  |

### Schritt 6: ICs und Optokoppler
Achte beim Einsetzen von ICs auf die Orientierung (Kerbe oder Punktmarkierung).

| Bauteil | Wert         | Typ          |
|---------|--------------|--------------|
| IC2     | TBD62003APG  | Treiber-IC   |
| OK1     | 814B         | Optokoppler  |
| OK2     | VO3023       | Optokoppler  |

### Schritt 7: Transistor bestücken
Setze den Transistor entsprechend der Form der Markierung auf der Platine ein.

| Bauteil | Wert            | Typ         |
|---------|-----------------|-------------|
| Q2      | GOFORD G01N20RE | Transistor  |

### Schritt 8: Netzteil-Modul bestücken

| Bauteil | Wert               | Typ            |
|---------|--------------------|----------------|
| PS2     | R02-T2S05-H        | Netzteilmodul  |

### Schritt 9: ESP32-Modul bestücken
Das Modul U1 (ESP32-DEVKITC) wird steckbar montiert. Löte dazu zwei 19-polige Buchsenleisten an den vorgesehenen Stellen auf der Platine ein. Anschließend kannst du das ESP32-Modul einfach aufstecken und bei Bedarf austauschen.

### Schritt 10: One-Wire-Bus für Temperatursensor (Optional)
Der One-Wire-Bus ermöglicht dir den Anschluss eines digitalen Temperatursensors. Löte R4 und SV4 ein. SV4 wird auf der Rückseite der Platine montiert, wobei die Steckernasen nach unten zeigen.

| Bauteil | Wert | Typ              | Farbringe                       |
|---------|------|------------------|---------------------------------|
| R4      | 4k7  | Widerstand       | Gelb - Violett - Rot - Gold     |
| SV4     | -    | Buchsenleiste    | -                               |


### Schritt 11: Ansteuerung für externen Schütz (Optional)
Diese Option ermöglicht dir die Ansteuerung eines externen Schützes oder Relais. R8, S1, R9 und JP2 müssen montiert werden. JP2 wird auf der Rückseite montiert, wobei die Rundungen nach oben zeigen.

| Bauteil | Wert     | Typ              | Farbringe                       |
|---------|----------|------------------|---------------------------------|
| R8      | 440Ω     | Widerstand       | Gelb - Gelb - Braun - Gold      |
| S1      | CPC1972G | Halbleiterrelais | -
| R9      | 10D561K  | Varistor         | -
| JP2     | -        | Anschlussblock   | -

### Schritt 12: Triac für Wellenpaketsteuerung (Optional)
Die Wellenpaketsteuerung ermöglicht ein noch feinere Steuerung in kleinen Leistungsstufen. Montiere hierzu R7 und OK3. Aktuell ist leider noch nicht implementiert und getestet. Also Experimentel.

| Bauteil | Wert     | Typ              | Farbringe                       |
|---------|----------|------------------|---------------------------------|
| R7      | 440Ω     | Widerstand       | Gelb - Gelb - Braun - Gold      |
| OK3     | VO3023   | Optokoppler      | -                               |

## Schritt 13: Endkontrolle
- Prüfe optisch alle Lötstellen.
- Kontrolliere nochmals die Polarität von Dioden, Kondensatoren und ICs.

# Schritt-für-Schritt-Bestückung "PTHC_REL_BRD"

## Leiterplatte Oberseite:

![drawing top](REL_BRDV4.1_TOP.png)

### Schritt 1: Widerstände (optional für Triac-Steuerung)

| Bauteil | Wert | Typ        | Hinweise          |
|---------|------|------------|-------------------|
| R1      | 330Ω | Widerstand | Optional, Triac   |
| R2      | 330Ω | Widerstand | Optional, Triac   |

Setze die Widerstände entsprechend der Markierung auf der Platine ein und kürze anschließend die überstehenden Beinchen.

### Schritt 2: Varistor bestücken (optional für Triac-Steuerung)

| Bauteil | Wert    | Typ      |
|---------|---------|----------|
| R3      | S10K11  | Varistor |

Der Varistor schützt die Schaltung vor Überspannungen. Die Einbaurichtung ist beliebig.

### Schritt 3: Triac (optional)

| Bauteil | Wert      | Typ   | Hinweise                      |
|---------|-----------|-------|-------------------------------|
| T1      | T3035H-6T | TRIAC | Falls nicht bestückt: brücken |

Solltest du den Triac nicht bestücken, setze eine Drahtbrücke anstelle des Triacs ein.

### Schritt 4: Temperatursicherung (Wichtig!)

| Bauteil | Wert | Hinweise                            |
|---------|------|-------------------------------------|
| TCO     | V0   | Beim Löten kurz löten und kühlen!   |

Achtung: **TCO darf nicht heiß werden**, ansonsten löst die Sicherung aus. Kurz und zügig löten, ggf. mit einem feuchten Tuch kühlen.

### Schritt 5: Lötbrücke SJ1 (optional)

SJ1 ist kein Bauteil, sondern eine Lötbrücke. Diese kann geschlossen werden, falls kein externer Schütz zur Netztrennung verwendet wird. Falls ein externer Schütz vorhanden ist, verbinde den Anschluss L1_C mit L1, der nicht vom Schütz getrennt wird.

### Schritt 6: Relais bestücken

| Bauteil | Typ   |
|---------|-------|
| K1-K5   | G2RE  |
| K7, K8  | G5LE  |

Relais können nur in einer Richtung eingesetzt werden, beachte die Markierung auf der Platine.

### Schritt 7: Sicherung einsetzen

| Bauteil | Wert      | Hinweise    |
|---------|-----------|-------------|
| F1      | FH1-206Z  | Sicherung   |

Setze die Sicherung fest ein und verlöte sie.

### Schritt 8: Schraubklemmen bestücken

| Bauteil | Typ            | Hinweise               |
|---------|----------------|------------------------|
| J1 - J5 | HB9500-9.5-2P  | Auf festen Sitz achten |

### Schritt 9: Stiftleisten vorbereiten

- Die Stiftleisten (SV1 und SV2) kommen als ein Stück und müssen gebrochen werden:
  - 1x 4-Pin (SV1)
  - 1x 8-Pin (SV2)

Verlöte die vorbereiteten Stiftleisten auf der Platine.

### Schritt 10: Control Board montieren

- Schiebe vorsichtig das \"Control Board\" auf die gewinkelten Stiftleisten und verlöte diese anschließend. Achte dabei das beiden Leiterplatten ordentlich im 90° Winkel stehen.

## Schritt 11: Endkontrolle

- Prüfe optisch alle Lötstellen.
- Kontrolliere nochmals den korrekten Sitz der Bauteile.
- Stelle sicher, dass keine Kurzschlüsse entstanden sind.

# Montage und Installationsanleitung

## Schritt 1: ESP32-Modul vorbereiten
ESP32-Modul Programmieren und am besten gleich mit eurem WLan Verbinden. Der Zugang zum Modul ist im eingebauten Zustand nich mehr so einfach.
Modul anschließend in die vorbereitete Buchsenleiste einsetzen.

## Schritt 2: Gehäusedurchbrüche erstellen
- Notwendige Durchbrüche im Gehäuse erstellen.
- Möglich sind: 4x M12, 3x M16/20, 1x M25/32. Achtung: Nicht alle Durchbrüche sind gleichzeitig möglich, da sie teilweise durch die Platinen verdeckt werden.
- Für die Ausschalter-Option wird ein M12-Durchbruch benötigt.
- Kabelverschraubungen einsetzen.

## Schritt 3: Elektrische Voraussetzungen
- Heizstab und Boiler erden.
- FI-Schutzschalter und entsprechende Sicherung je nach Heizstab (max. 25A) vorschalten.
- Lokale Vorschriften beachten!

### Querschnittstabelle Zuleitung (3-phasig im Installationskanal)
| Heizstab Leistung (kW) | Sicherung (A) | Mindestquerschnitt (mm²) |
|------------------------|---------------|---------------------------|
| 3                      | 6             | 1,5                       |
| 6                      | 10            | 1,5                       |
| 9                      | 16            | 2,5                       |
| 12                     | 20            | 2,5                       |
| 15                     | 25            | 4                         |

## Schritt 4: Heizstab-Verdrahtung
Alle Brückenplättchen vom Heizstab entfernen.Es ist ein 7-poliges Kabel für die Heizstabverdrahtung notwendig. Der Schutzerde (PE) muss unbedingt gemäß den Vorschriften angeschlossen werden!
Achtung: Die Kabeltemperatur beeinflusst den notwendigen Mindestquerschnitt. Je nach örtlichen Bedingungen muss der Querschnitt ggf. erhöht werden.

### Tabelle Heizstabverdrahtung
| Leistung (kW) | Phasenstrom (A) | Strangstrom (A) | Mindestquerschnitt Heizstab (mm²) |
|---------------|-----------------|-----------------|-----------------------------------|
| 3             | 4,3             | 2,5             | 1,0                               |
| 6             | 8,7             | 5,0             | 1,5                               |
| 9             | 13              | 7,5             | 1,5                               |
| 12            | 17,3            | 10              | 2,5                               |
| 15            | 21,7            | 12,5            | 4                                 |

**Hinweis:** Der Strangstrom ist √3 kleiner als der Phasenstrom, daher kann der Querschnitt evtl. kleiner als bei der Zuleitung gewählt werden.

## Schritt 5: Anschluss Heizstab
Mit einem Multimeter die drei Heizstäbe bestimmen. Jeder der drei Heizstäbe hat zwei Anschlüsse. Die Anschlussbezeichnung am Platinensatz ist dann entsprechen Hx.y, wobei x ein Heizstab ist und y ein Anschluss dessen.
z.B. bedeutet H1.1 Heizstab 1 Anschluss 1. Alle Heizstäbe sind equivalent d.h. ihr könnt selbst festlegen welcher Heizstab euere Nummer 1 ist ;-).

## Schritt 6: Zuleitung und Drehrichtung
Zuleitung anschließen und die richtige Drehrichtung prüfen (z.B. mit SM852B). Das ist wichtig da nur Phase L1 wird gemessen wird und alle anderen Phasen werden daraus abgeleitet. Ohne korrekte Drehrichtung funktioniert die Nulldurchgangserkennung nur für Phase L1. Die Relais werden bei korrekter Installation im Nulldurchgang geschaltet, da in diesem moment kein Strom fließt ist das besonders Kontaktschonend.
L2 und H3.2 werden direkt verbunden, ohne externen Trennschütz liegt immer, auch im ausgeschalteten Zustand das Potential von L2 am Heizstab an! Deshalb vor Arbeiten immer stromlos schalten!
Bei externer Trennschütz Option muss L1_C mit nicht geschaltener L1 verbunden werden. Um die Versorgung der Steuerung selbst sicher zu stellen.

## Fertigstellung

Glückwunsch! Hoffentlich funktioniert alles so wie du möchtest.


