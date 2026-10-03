/*
  Copyright (C) 2026  Ratmir Timergaliev
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://gnu.org>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

#include <ncurses.h>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

void clearScreen()
{
    erase();
    clearok(stdscr, TRUE);
    refresh();
}

void launchGryphDM()
{
    clearScreen();
    endwin();

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        return;
    }

    if (pid == 0) {

        execlp(
            "gryphdm",
            "gryphdm",
            (char *)NULL
        );

        perror("Failed to start gryphdm");

        _exit(127);
    }

    int status;

    waitpid(
        pid,
        &status,
        0
    );

    clearScreen();
}


// ============================================================
// HELP
// ============================================================

void showHelp()
{
    vector<string> helpText = {

        "Gryphon-Installer Help",
        "",
        "Gryphon-Installer is the installation and recovery",
        "environment for Gryphon Linux.",
        "",
        "Main Menu",
        "---------",
        "",
        "UP / DOWN",
        "    Select an item from the menu.",
        "",
        "ENTER",
        "    Open the selected item.",
        "",
        "Q",
        "    Quit Gryphon-Installer.",
        "",
        "",
        "Disk Utility",
        "------------",
        "",
        "Disk Utility launches Gryphon Disk Management (gryphdm).",
        "",
        "gryphi will wait while gryphdm is running.",
        "After you exit gryphdm, you will automatically return",
        "to the Gryphon-Installer main menu.",
        "",
        "",
        "Install / Recovery",
        "------------------",
        "",
        "This section is reserved for installation and recovery",
        "operations.",
        "",
        "",
        "Navigation",
        "----------",
        "",
        "Use the UP and DOWN arrow keys to scroll.",
        "",
        "Press Q to return to the main menu."
    };


    int scroll = 0;
    int ch;

    while (true) {

        erase();

        int height;
        int width;

        getmaxyx(
            stdscr,
            height,
            width
        );

        box(
            stdscr,
            0,
            0
        );


        mvprintw(
            0,
            2,
            " Help "
        );

        int visibleLines = height - 4;

        if (visibleLines < 1)
            visibleLines = 1;


        int maxScroll =
            static_cast<int>(helpText.size())
            - visibleLines;


        if (maxScroll < 0)
            maxScroll = 0;


        if (scroll > maxScroll)
            scroll = maxScroll;


        for (int i = 0; i < visibleLines; i++) {

            int line = scroll + i;

            if (line >= helpText.size())
                break;


            mvprintw(
                i + 1,
                2,
                "%.*s",
                width - 4,
                helpText[line].c_str()
            );
        }

        mvprintw(
            height - 2,
            2,
            "↑↓ Scroll    q Back"
        );


        refresh();

        ch = getch();


        switch (ch) {

            case KEY_UP:

                if (scroll > 0)
                    scroll--;

                break;


            case KEY_DOWN:

                if (scroll < maxScroll)
                    scroll++;

                break;


            case 'q':
            case 'Q':

                erase();

                clearok(
                    stdscr,
                    TRUE
                );

                refresh();

                return;


            default:
                break;
        }
    }
}

int main(int argc, char **argv)
{

    initscr();

    noecho();

    curs_set(0);

    keypad(
        stdscr,
        true
    );

    vector<string> options = {
        "Help",
        "Install/Recovery",
        "Disk Utility"
    };


    int highlight = 0;
    int option;


    while (true) {

        erase();


        int xm;
        int ym;

        getmaxyx(
            stdscr,
            ym,
            xm
        );

        int menuHeight = 14;

        int menuWidth = xm / 2;


        if (menuWidth < 50)
            menuWidth = 50;


        if (menuWidth > xm - 2)
            menuWidth = xm - 2;


        int menuY =
            ym / 2
            - menuHeight / 2;


        int menuX =
            xm / 2
            - menuWidth / 2;

        WINDOW *mwin = newwin(
            menuHeight,
            menuWidth,
            menuY,
            menuX
        );


        keypad(
            mwin,
            true
        );

        box(
            mwin,
            0,
            0
        );

        mvwprintw(
            mwin,
            0,
            2,
            " Gryphon-Installer "
        );

        mvwprintw(
            mwin,
            2,
            2,
            "Welcome to the Install/Recovery tool"
        );


        mvwprintw(
            mwin,
            3,
            2,
            "for Gryphon Linux."
        );


        mvwprintw(
            mwin,
            4,
            2,
            "Select the action you want to perform."
        );

        for (int i = 0; i < options.size(); i++) {

            if (i == highlight) {

                wattron(
                    mwin,
                    A_REVERSE
                );


                mvwprintw(
                    mwin,
                    i + 6,
                    4,
                    " %s ",
                    options[i].c_str()
                );


                wattroff(
                    mwin,
                    A_REVERSE
                );
            }

            else {

                mvwprintw(
                    mwin,
                    i + 6,
                    4,
                    " %s ",
                    options[i].c_str()
                );
            }
        }

        mvwprintw(
            mwin,
            menuHeight - 3,
            2,
            "↑↓ Select   Enter Select   q Quit"
        );


        wrefresh(
            mwin
        );

        option = wgetch(
            mwin
        );

        delwin(
            mwin
        );


        erase();


        switch (option) {

            case KEY_UP:

                highlight--;

                if (highlight < 0)
                    highlight = options.size() - 1;

                break;


            case KEY_DOWN:

                highlight++;

                if (highlight >= options.size())
                    highlight = 0;

                break;

            case 10:

                if (options[highlight] == "Help") {

                    showHelp();

                    erase();

                    clearok(
                        stdscr,
                        TRUE
                    );

                    refresh();
                }

                else if (
                    options[highlight]
                    == "Install/Recovery"
                ) {

                    erase();

                    mvprintw(
                        2,
                        2,
                        "Install/Recovery is not implemented yet."
                    );


                    mvprintw(
                        4,
                        2,
                        "Press any key to return..."
                    );


                    refresh();

                    getch();


                    erase();

                    clearok(
                        stdscr,
                        TRUE
                    );

                    refresh();
                }

                else if (
                    options[highlight]
                    == "Disk Utility"
                ) {

                    launchGryphDM();

                    erase();

                    clearok(
                        stdscr,
                        TRUE
                    );

                    refresh();
                }

                break;


            // ------------------------------------------------
            // QUIT
            // ------------------------------------------------

            case 'q':
            case 'Q':

                endwin();

                return 0;


            default:
                break;
        }
    }


    endwin();

    return 0;
}