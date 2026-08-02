#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Tkinter GUI for PickleBot.

Launches the picklebot binary as a subprocess, reads its `@BOARD@` frames
and log lines, and sends moves to its standard input - the same pattern as
the Decoder-Malfunction-Simulator's dekoder_gui.py.
"""

import os
import queue
import re
import subprocess
import sys
import threading
import tkinter as tk
from tkinter import messagebox, ttk

ANSI = re.compile(r'\x1b\[[0-9;]*[A-Za-z]')

KATALOG = os.path.dirname(os.path.abspath(__file__))
BINAR = os.path.join(KATALOG, 'picklebot')

KOLOR = {'light': '#eeeed2', 'dark': '#769656'}
KOLOR_ZAZNACZENIA = '#f0c040'
TEXT_LIGHT = '#1c1c1c'
TEXT_DARK = '#f8f8f8'
TLO = '#101418'

GLYPHY = {
    'P': '\u2659', 'N': '\u2658', 'B': '\u2657', 'R': '\u2656',
    'Q': '\u2655', 'K': '\u2654',
    'p': '\u265f', 'n': '\u265e', 'b': '\u265d', 'r': '\u265c',
    'q': '\u265b', 'k': '\u265a',
}

KOLOR_LOGU = {
    'ok': '#35d07f', 'err': '#ff5d5d', 'warn': '#ffd166',
    'info': '#4dd2ff', 'txt': '#c8d6e0',
}

# bot modes: number used by the engine, display name and short description
TRYBY = [
    {'nr': 1, 'pl': 'Agresywny (materiał)', 'en': 'Aggressive (material)',
     'pl_opis': 'Poluje na materiał wroga.', 'en_opis': 'Hunts enemy material.'},
    {'nr': 2, 'pl': 'Ofensywny (król)', 'en': 'Offensive (king)',
     'pl_opis': 'Atakuje króla przeciwnika.', 'en_opis': 'Attacks the enemy king.'},
    {'nr': 3, 'pl': 'Defensywny (materiał)', 'en': 'Defensive (material)',
     'pl_opis': 'Broni się i trzyma materiał.', 'en_opis': 'Defends, keeps material safe.'},
    {'nr': 4, 'pl': 'Ochronny (król)', 'en': 'Guarding (king)',
     'pl_opis': 'Najpierw broni własnego króla.', 'en_opis': 'Defends its own king above all.'},
]


def poziom_logu(tekst):
    if '[ OK ]' in tekst:
        return 'ok'
    if '[ !! ]' in tekst:
        return 'err'
    if '[ ! ]' in tekst:
        return 'warn'
    if '[ -> ]' in tekst:
        return 'info'
    return 'txt'


class Aplikacja:
    def __init__(self, jezyk_en=False):
        self.root = tk.Tk()
        self.root.title('PickleBot - panel')
        self.root.configure(bg=TLO)
        self.en = bool(jezyk_en)
        self.q = queue.Queue()
        self.proc = None
        self.pola = [None] * 64
        self.zaznaczone = None      # index of the selected square, or None
        self.obecna_plansza = None  # last parsed @BOARD@ (64 chars)
        self.player_white = None    # player's colour from @SIDE@
        self.tura_biala = None      # whose turn, from @BOARD@
        self.gra_aktywna = False
        self._styl()
        self._buduj_szachownice()
        self._buduj_panel()
        self._odswiez_jezyk()
        self.root.protocol('WM_DELETE_WINDOW', self.zamknij)
        self.root.after(50, self._przetwarzaj_kolejke)

    def _t(self, pl, en):
        return en if self.en else pl

    def _nazwa_trybu(self, nr):
        t = TRYBY[nr - 1]
        return t['en'] if self.en else t['pl']

    def _opis_trybu(self, nr):
        t = TRYBY[nr - 1]
        return t['en_opis'] if self.en else t['pl_opis']

    def _numer_trybu(self):
        for t in TRYBY:
            if self._nazwa_trybu(t['nr']) == self.var_tryb.get():
                return t['nr']
        return 1

    def _wybrano_tryb(self):
        self.nr_trybu = self._numer_trybu()
        self._odswiez_opis_trybu()

    def _odswiez_tryby(self):
        nazwy = [self._nazwa_trybu(t['nr']) for t in TRYBY]
        self.tryby['values'] = nazwy
        self.var_tryb.set(self._nazwa_trybu(self.nr_trybu))
        self._odswiez_opis_trybu()

    def _odswiez_opis_trybu(self):
        self.etykieta_opis_trybu.configure(text=self._opis_trybu(self.nr_trybu))

    def _styl(self):
        self.s = ttk.Style(self.root)
        try:
            self.s.theme_use('clam')
        except tk.TclError:
            pass
        self.s.configure('TFrame', background=TLO)
        self.s.configure('TLabel', background=TLO, foreground='#c8d6e0')
        self.s.configure('TButton', background='#1c2229', foreground='#c8d6e0')

    def _buduj_szachownice(self):
        ramka = tk.Frame(self.root, bg=TLO)
        ramka.pack(side='left', padx=12, pady=12)
        for i in range(64):
            ciemne = (i // 8 + i % 8) % 2 == 0
            e = tk.Label(ramka, width=3, height=1, font=('DejaVu Sans', 22),
                         bg=KOLOR['dark'] if ciemne else KOLOR['light'],
                         fg=TEXT_DARK if ciemne else TEXT_LIGHT)
            e.grid(row=i // 8, column=i % 8)
            e.bind('<Button-1>', lambda _e, i=i: self._klik(i))
            self.pola[i] = e

    def _buduj_panel(self):
        panel = tk.Frame(self.root, bg=TLO)
        panel.pack(side='left', fill='y', padx=(0, 12), pady=12)

        self.var_tryb = tk.StringVar()
        self.var_strona = tk.StringVar(value='W')
        self.var_czas = tk.StringVar(value='0')
        self.var_verbose = tk.BooleanVar(value=False)

        self.etykiety = []
        self.etykiety.append(tk.Label(panel, text='', bg=TLO, fg='#c8d6e0'))

        self.tryby = ttk.Combobox(panel, textvariable=self.var_tryb, state='readonly', width=26)
        self.tryby.bind('<<ComboboxSelected>>', lambda _e: self._wybrano_tryb())
        self.nr_trybu = 1
        self.etykieta_opis_trybu = tk.Label(panel, text='', bg=TLO, fg='#9fb0bd',
                                            wraplength=230, justify='left')

        strony = ttk.Combobox(panel, textvariable=self.var_strona, state='readonly', width=22)
        strony['values'] = ['W', 'B', 'R']
        strony.current(0)

        czas = tk.Spinbox(panel, from_=0, to=600, textvariable=self.var_czas, width=22)

        self.verbose_ck = tk.Checkbutton(panel, bg=TLO, fg='#c8d6e0', selectcolor='#1c2229',
                                         variable=self.var_verbose)

        self.etykieta_status = tk.Label(panel, text='', bg=TLO, fg='#ffd166',
                                        font=('DejaVu Sans', 10, 'bold'))

        self.przycisk_start = tk.Button(panel, command=self._start_procesu, width=22)
        self.przycisk_jezyk = tk.Button(panel, command=self._przelacz_jezyk, width=22)
        self.przycisk_zamknij = tk.Button(panel, command=self.zamknij, width=22)

        self.pole_we = tk.Entry(panel, width=24, bg='#1c2229', fg='#c8d6e0',
                                insertbackground='#c8d6e0')
        self.pole_we.bind('<Return>', lambda _e: self.wyslij())
        self.przycisk_wyslij = tk.Button(panel, command=self.wyslij, width=22)

        self.konsola = tk.Text(panel, width=58, height=24, bg='#0c1014', fg='#c8d6e0',
                               state='disabled', wrap='word', font=('DejaVu Sans Mono', 9))
        for nazwa, kolor in KOLOR_LOGU.items():
            self.konsola.tag_configure(nazwa, foreground=kolor)

        wiersz = 0
        self.etykiety[0].grid(row=wiersz, column=0, columnspan=2, pady=(0, 4))
        wiersz += 1
        self._etykieta_pola(panel, 'tryb', wiersz)
        self.tryby.grid(row=wiersz, column=1, pady=2, sticky='w')
        wiersz += 1
        self.etykieta_opis_trybu.grid(row=wiersz, column=0, columnspan=2, pady=(0, 4), sticky='w')
        wiersz += 1
        self._etykieta_pola(panel, 'strona', wiersz)
        strony.grid(row=wiersz, column=1, pady=2, sticky='w')
        wiersz += 1
        self._etykieta_pola(panel, 'czas', wiersz)
        czas.grid(row=wiersz, column=1, pady=2, sticky='w')
        wiersz += 1
        self._etykieta_pola(panel, 'verbose', wiersz)
        self.verbose_ck.grid(row=wiersz, column=1, pady=2, sticky='w')
        wiersz += 1
        self.etykieta_status.grid(row=wiersz, column=0, columnspan=2, pady=(2, 6))
        wiersz += 1
        self.przycisk_start.grid(row=wiersz, column=0, columnspan=2, pady=3)
        wiersz += 1
        self.przycisk_jezyk.grid(row=wiersz, column=0, columnspan=2, pady=3)
        wiersz += 1
        self.przycisk_zamknij.grid(row=wiersz, column=0, columnspan=2, pady=3)
        wiersz += 1
        self._etykieta_pola(panel, 'ruch', wiersz)
        self.pole_we.grid(row=wiersz, column=1, pady=(8, 2), sticky='w')
        wiersz += 1
        self.przycisk_wyslij.grid(row=wiersz, column=0, columnspan=2, pady=(0, 8))
        wiersz += 1
        self.konsola.grid(row=wiersz, column=0, columnspan=2, sticky='nsew')
        panel.rowconfigure(wiersz, weight=1)

    def _etykieta_pola(self, panel, klucz, wiersz):
        e = tk.Label(panel, text='', bg=TLO, fg='#9fb0bd')
        e.grid(row=wiersz, column=0, pady=2, sticky='e')
        self.etykiety.append((klucz, e))

    def _odswiez_jezyk(self):
        self.root.title('PickleBot - ' + self._t('panel', 'panel'))
        napisy = {
            'tryb': self._t('Tryb bota:', 'Bot mode:'),
            'strona': self._t('Twoja strona:', 'Your side:'),
            'czas': self._t('Czas (s/strona):', 'Time (s/side):'),
            'verbose': self._t('Tryb verbose', 'Verbose mode'),
            'ruch': self._t('Ruch (np. e2 e4):', 'Move (e.g. e2 e4):'),
        }
        if self.etykiety:
            self.etykiety[0].configure(text=self._t(
                'PICKLEBOT - szachy vs bot',
                'PICKLEBOT - chess vs the bot'))
        for klucz, e in self.etykiety[1:]:
            e.configure(text=napisy.get(klucz, ''))
        self.przycisk_start.configure(text=self._t('START', 'START'))
        self.przycisk_jezyk.configure(text=self._t('ENGLISH', 'POLSKI'))
        self.przycisk_zamknij.configure(text=self._t('WYJDŹ', 'QUIT'))
        self.przycisk_wyslij.configure(text=self._t('WYŚLIJ', 'SEND'))
        self._ustaw_status()
        self._odswiez_tryby()

    def _przelacz_jezyk(self):
        self.en = not self.en
        self._odswiez_jezyk()

    def _start_procesu(self):
        if self.proc is not None and self.proc.poll() is None:
            return
        if not os.path.exists(BINAR):
            komunikat = self._t(
                ('Nie znaleziono programu:\n' + BINAR +
                 '\n\nNajpierw zbuduj go:\n  g++ -O2 -D_FORTIFY_SOURCE=2 '
                 '-fstack-protector-all -std=c++17 -Wall -Wextra '
                 'main.cpp board.cpp evaluate.cpp search.cpp lang.cpp -o picklebot'),
                ('Program not found:\n' + BINAR +
                 '\n\nBuild it first:\n  g++ -O2 -D_FORTIFY_SOURCE=2 '
                 '-fstack-protector-all -std=c++17 -Wall -Wextra '
                 'main.cpp board.cpp evaluate.cpp search.cpp lang.cpp -o picklebot'))
            print('BLAD: ' + komunikat, file=sys.stderr)
            messagebox.showerror(self._t('Brak picklebota', 'PickleBot missing'), komunikat)
            return
        args = [BINAR, '--mode', str(self._numer_trybu()), '--side', self.var_strona.get(),
                '--time', self.var_czas.get()]
        args.append('--verbose' if self.var_verbose.get() else '--noverbose')
        if self.en:
            args.append('--en')
        try:
            self.proc = subprocess.Popen(
                args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT)
        except OSError as e:
            messagebox.showerror(self._t('Błąd', 'Error'),
                                 self._t('Nie udało się uruchomić picklebota: ' + str(e),
                                          'Could not start picklebot: ' + str(e)))
            return
        self._dopisz_log(self._t('Uruchomiono picklebota.', 'PickleBot started.'), 'info')
        self.gra_aktywna = True
        self.konsola.configure(state='normal')
        self.konsola.delete('1.0', 'end')
        self.konsola.configure(state='disabled')
        threading.Thread(target=self._czytaj, daemon=True).start()

    def _czytaj(self):
        p = self.proc.stdout
        while True:
            linia = p.readline()
            if not linia:
                break
            self._linia(linia.decode('utf-8', 'replace'))
        self.q.put(('eof',))

    def _linia(self, surowa):
        w = surowa.rstrip('\r\n')
        if w.startswith('@BOARD@ '):
            reszta = w[len('@BOARD@ '):]
            tura = reszta[0:1]
            plansza = reszta[2:2 + 64]
            if tura in ('W', 'B') and len(plansza) == 64:
                self.q.put(('board', tura, plansza))
        elif w.startswith('@SIDE@ '):
            self.q.put(('side', w[len('@SIDE@ '):].strip()))
        else:
            tekst = ANSI.sub('', w).strip()
            if tekst:
                self.q.put(('log', tekst))

    def wyslij(self, czysc_pole=True):
        komenda = self.pole_we.get().strip()
        if not komenda or not self.proc or self.proc.poll() is not None:
            return
        self._dopisz_log('>> ' + komenda, 'info')
        try:
            self.proc.stdin.write((komenda + '\n').encode('utf-8'))
            self.proc.stdin.flush()
        except (BrokenPipeError, OSError):
            pass
        if czysc_pole:
            self.pole_we.delete(0, 'end')

    def _przetwarzaj_kolejke(self):
        try:
            while True:
                item = self.q.get_nowait()
                typ = item[0]
                if typ == 'board':
                    self.tura_biala = item[1] == 'W'
                    self.zaznaczone = None
                    self._rysuj_plansze(item[2])
                    self._echo_plansza(item[2])
                    self._ustaw_status()
                elif typ == 'side':
                    self.player_white = item[1] == 'W'
                    self._ustaw_status()
                elif typ == 'log':
                    self._dopisz_log(item[1])
                elif typ == 'eof':
                    self.gra_aktywna = False
                    self._dopisz_log(self._t('Koniec strumienia - gra zakończona.',
                                             'Stream ended - the game is over.'), 'warn')
                    self._ustaw_status()
        except queue.Empty:
            pass
        self.root.after(50, self._przetwarzaj_kolejke)

    def _rysuj_plansze(self, plansza):
        self.obecna_plansza = plansza
        for i, znak in enumerate(plansza):
            self.pola[i].configure(text=GLYPHY.get(znak, ''))
        self._podswietl()

    def _podswietl(self):
        for i in range(64):
            ciemne = (i // 8 + i % 8) % 2 == 0
            bg = KOLOR['dark'] if ciemne else KOLOR['light']
            fg = TEXT_DARK if ciemne else TEXT_LIGHT
            if i == self.zaznaczone:
                bg = KOLOR_ZAZNACZENIA
                fg = '#1c1c1c'
            self.pola[i].configure(bg=bg, fg=fg)

    def _klik(self, indeks):
        if not self.gra_aktywna or self.proc is None or self.proc.poll() is not None:
            return
        if self.player_white is None or self.tura_biala is None or self.obecna_plansza is None:
            return
        if self.tura_biala != self.player_white:
            self._dopisz_log(self._t('Poczekaj na ruch bota...', 'Waiting for the bot...'), 'info')
            return
        if self.zaznaczone is None:
            znak = self.obecna_plansza[indeks]
            if znak == ' ':
                self._dopisz_log(self._t('Wybierz własną figurę.', 'Pick one of your pieces.'),
                                 'warn')
                return
            if znak.isupper() != self.player_white:
                self._dopisz_log(self._t('To nie twoja figura.', 'That is not your piece.'),
                                 'warn')
                return
            self.zaznaczone = indeks
            self._podswietl()
        else:
            skad = self.zaznaczone
            self.zaznaczone = None
            self._podswietl()
            if skad == indeks:
                return
            self.wyslij_ruch(skad, indeks)

    def _nazwa_pola(self, indeks):
        return 'abcdefgh'[indeks % 8] + str(8 - indeks // 8)

    def wyslij_ruch(self, skad, dokad):
        if not self.proc or self.proc.poll() is not None:
            return
        m = self._nazwa_pola(skad) + ' ' + self._nazwa_pola(dokad)
        self._dopisz_log('>> ' + m, 'info')
        try:
            self.proc.stdin.write((m + '\n').encode('utf-8'))
            self.proc.stdin.flush()
        except (BrokenPipeError, OSError):
            pass

    def _ustaw_status(self):
        if not hasattr(self, 'etykieta_status'):
            return
        if not self.gra_aktywna:
            txt = self._t('Naciśnij START.', 'Press START.')
        elif self.player_white is None or self.tura_biala is None:
            txt = self._t('Uruchamianie...', 'Starting...')
        elif self.tura_biala == self.player_white:
            txt = self._t('Twoja kolej - kliknij figurę.', 'Your turn - click a piece.')
        else:
            txt = self._t('Bot myśli...', 'Bot is thinking...')
        self.etykieta_status.configure(text=txt)

    def _echo(self, tekst):
        print(tekst, flush=True)

    def _echo_plansza(self, plansza):
        for r in range(8):
            self._echo(' '.join(plansza[r * 8:(r + 1) * 8]))
        self._echo('')

    def _dopisz_log(self, tekst, poz=None):
        if poz is None:
            poz = poziom_logu(tekst)
        self._echo(tekst)
        self.konsola.configure(state='normal')
        self.konsola.insert('end', tekst + '\n', poz)
        self.konsola.see('end')
        self.konsola.configure(state='disabled')

    def zamknij(self):
        if self.proc is not None and self.proc.poll() is None:
            try:
                self.proc.stdin.write(b'quit\n')
                self.proc.stdin.flush()
            except (BrokenPipeError, OSError):
                pass
            try:
                self.proc.wait(timeout=4)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.root.destroy()


def main():
    jezyk_en = not ('--pl' in sys.argv or '--polski' in sys.argv)
    try:
        Aplikacja(jezyk_en=jezyk_en)
    except tk.TclError as e:
        print('BLAD: nie mozna otworzyc okna graficznego.', file=sys.stderr)
        print('  Szczegoly: %s' % e, file=sys.stderr)
        print('  Aplikacja Tkinter wymaga dzialajacego srodowiska graficznego (X).', file=sys.stderr)
        print('  Uruchom z terminala w sesji graficznej, np.:', file=sys.stderr)
        print('      python3 picklebot_gui.py', file=sys.stderr)
        sys.exit(1)
    tk.mainloop()


if __name__ == '__main__':
    main()
