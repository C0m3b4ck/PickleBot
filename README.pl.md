# PickleBot
![wersja](https://img.shields.io/badge/wersja-1.0-blue) ![jezyk](https://img.shields.io/badge/jezyk-polski-green) ![cpp](https://img.shields.io/badge/silnik-C%2B%2B17-blue) ![python](https://img.shields.io/badge/gui-Python%203%20%2B%20Tkinter-green)

**🇵🇱🇵🇱🇵🇱Zrobione w Polsce🇵🇱🇵🇱🇵🇱**
[🇬🇧 Read in English!](README.md)

**Bot szachowy napisany w C++, używający algorytmów i metod oceny pozycji zamiast wytrenowanego modelu ML.**

Stworzony z myślą o środowiskach o ograniczonych zasobach, każde forki (zwłaszcza unikalne) mile widziane.

**Polski** · [English](README.md)

## Jak to działa

### Reprezentacja szachownicy

Pozycja jest strukturą `Board` zawierającą tablicę 8x8 figur, pole docelowe bicia w przelocie (en passant) oraz cztery prawa do roszady. Pola są indeksowane `a8 = 0` ... `h1 = 63`.

### Generowanie ruchów

Każdy kandydat na ruch jest generowany dla każdej figury i walidowany pod kątem:

- standardowych zasad ruchu (pion, skoczek, goniec, wieża, hetman, król, en passant, roszada),
- tego, czy ruch nie zostawia własnego króla pod szachem.

Przeszukiwane i wykonywane są wyłącznie w pełni legalne ruchy.

### Ocena pozycji

Statyczna ocena łączy cztery komponenty:

| Komponent   | Znaczenie                                            |
|-------------|------------------------------------------------------|
| Materiał    | wartość figur (`P=1, S/G=3, W=5, H=9`) przeskalowana |
| Bliskość    | jak blisko króla przeciwnika znajdują się figury bota |
| Szach       | danie szacha królowi przeciwnika                     |
| Odkrycie    | ruch odsłaniający króla bota                         |

Funkcja `weighted()` łączy je w jeden wynik zgodnie z aktywnym **trybem bota** (osobowością), więc ten sam silnik gra inaczej w zależności od wybranego trybu.

### Przeszukiwanie

- **Negamax** z **obcinaniem alfa-beta**, do głębokości 4 domyślnie (do 6).
- **Sortowanie ruchów** według szacunkowej wartości (najpierw bicia) dla lepszego obcinania.
- **Pogłębianie iteracyjne**: głębokość rośnie tura po turze, aż wyczerpie się budżet czasu; zachowana jest ostatnia ukończona głębokość, więc zawsze gra na czas.
- **Zarządzanie czasem**: budżet tury to ułamek pozostałego czasu na zegarze strony, z dolnym limitem, więc silnik nigdy się nie zacina.
- **Przewidywanie odpowiedzi przeciwnika** (tryb verbose): po wybraniu ruchu silnik przeszukuje kilka posunięć głębiej, aby przewidzieć prawdopodobną odpowiedź przeciwnika.

### Tryby bota

1. **Agresywny** – poluje na materiał.
2. **Ofensywny** – koncentruje się na królu przeciwnika.
3. **Defensywny** – chroni własną pozycję.
4. **Ochronny** – defensywny nacisk na króla.

### Dwujęzyczność

🇵🇱🇬🇧 Interfejs jest **dwujęzyczny** — domyślnie polski, w dowolnym momencie przełączysz go na angielski flagą `--en`, albo w trakcie gry wpisując `language en` / `language pl` (lub `jezyk en` / `jezyk pl`) w miejscu, gdzie wpisuje się ruch.

## Użycie

### Budowanie

Skompiluj kompilatorem C++17:

```sh
g++ -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -std=c++17 -Wall -Wextra \
    main.cpp board.cpp evaluate.cpp search.cpp lang.cpp -o picklebot
```

### Flagi linii poleceń

| Flaga | Działanie |
|-------|-----------|
| `--en`, `--ang`, `--english` | uruchomienie po angielsku |
| `--pl`, `--polski` | uruchomienie po polsku (domyślnie) |
| `--verbose`, `-v` | pokazuje myślenie bota |
| `--noverbose`, `--silent` | bez trybu verbose |
| `--mode N` | tryb bota 1-4 (agresywny / ofensywny / defensywny / ochronny) |
| `--side B\|C\|L` | twoja strona: Białe, Czarne lub Losowo |
| `--time N` | limit czasu w sekundach na stronę (0 = brak) |
| `--help`, `-h` | pokazanie pomocy i wyjście |

Na starcie zostaniesz zapytany o wszystko, czego nie podałeś flagami:

- **tryb bota** (1-4),
- twoją **stronę** (Białe / Czarne / Losowo),
- **limit czasu** w sekundach na stronę (0 = brak),
- czy włączyć **tryb verbose**.

Ruchy wpisuje się w notacji algebraicznej, np. `e2 e4`.

### GUI

GUI w Tkinter (wzorowany na `dekoder_gui.py` z Symulatora Awarii Dekodera) uruchamia silnik jako podproces:

```sh
g++ -O2 -D_FORTIFY_SOURCE=2 -fstack-protector-all -std=c++17 -Wall -Wextra \
    main.cpp board.cpp evaluate.cpp search.cpp lang.cpp -o picklebot
python3 picklebot_gui.py
```

Panel pozwala wybrać tryb bota, twoją stronę, limit czasu i tryb verbose, a potem pokazuje szachownicę, czyją jest tura, dziennik gry i pole na ruchy. Figurami ruszasz **klikając** je (wybierz figurę, potem kliknij pole docelowe) albo wpisując ruch do pola. Wszystko, co mówi silnik — i każda szachownica — jest też wypisywane do terminala. Uruchom z `--en`, aby zacząć po angielsku.

## TODO

Zobacz [TODO.txt](TODO.txt): rozdzielić na pliki, dokończyć logikę, rozszerzyć zmienne algorytmów i wagi.

## Kredyty

Zaczął 11 czerwca 2026 r., autor: C0m3b4ck. Przepisanie rozpoczęto 27 lipca. Jak dotąd nie ma innych współpracowników poza oryginalnym autorem.
