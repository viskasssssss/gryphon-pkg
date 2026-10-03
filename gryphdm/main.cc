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
#include <stdarg.h>

#include <sstream>
#include <vector>
#include <fstream>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <algorithm>

#include <ncurses.h>

struct block_device {
    std::string name;
    unsigned long major;
    unsigned long minor;
    unsigned long long blocks;
    bool is_partition;
};

struct device_info {
    std::string path;
    std::string name;
    std::string type;
    std::string size;
    std::string filesystem;
    std::string label;
    std::string uuid;
    std::string mountpoint;
    std::string model;
    std::string serial;

    bool read_only = false;
    bool removable = false;
};

enum class partition_table_type
{
    GPT,
    MBR,
    CANCEL
};

enum class filesystem_type
{
    EXT4,
    XFS,
    BTRFS,
    VFAT,
    SWAP,
    CANCEL
};

void die(const char* format, ...)
{
    if (!isendwin())
        endwin();

    va_list args;
    va_start(args, format);

    std::fprintf(stderr, "Error: ");
    std::vfprintf(stderr, format, args);
    std::fprintf(stderr, "\n");

    va_end(args);

    std::exit(EXIT_FAILURE);
}

void show_message(const std::string& title,
                  const std::string& message)
{
    int width = std::min(72, COLS - 4);

    if (width < 30)
        width = 30;

    std::vector<std::string> lines;

    std::istringstream input(message);
    std::string paragraph;

    while (std::getline(input, paragraph))
    {
        if (paragraph.empty())
        {
            lines.push_back("");
            continue;
        }

        while (static_cast<int>(paragraph.size()) > width - 6)
        {
            size_t break_pos =
                paragraph.rfind(' ', width - 6);

            if (break_pos == std::string::npos)
                break_pos = width - 6;

            lines.push_back(
                paragraph.substr(0, break_pos));

            paragraph.erase(0, break_pos);

            while (!paragraph.empty() &&
                   paragraph[0] == ' ')
            {
                paragraph.erase(0, 1);
            }
        }

        lines.push_back(paragraph);
    }

    int height =
        static_cast<int>(lines.size()) + 5;

    if (height > LINES - 2)
        height = LINES - 2;

    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;

    WINDOW* win =
        newwin(height, width, start_y, start_x);

    if (!win)
        return;

    box(win, 0, 0);

    mvwprintw(win, 1, 2, "%s", title.c_str());

    int row = 3;

    for (const std::string& line : lines)
    {
        if (row >= height - 2)
            break;

        mvwprintw(win, row, 2, "%s", line.c_str());
        row++;
    }

    mvwprintw(win,
              height - 2,
              2,
              "Press any key...");

    wrefresh(win);

    wgetch(win);

    delwin(win);
}

bool input_dialog(const std::string& title,
                  const std::string& prompt,
                  std::string& result)
{
    int height = 10;
    int width = 60;

    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;

    WINDOW* win =
        newwin(height, width, start_y, start_x);

    if (!win)
        return false;

    box(win, 0, 0);

    mvwprintw(win,
              1,
              2,
              "%s",
              title.c_str());

    std::istringstream stream(prompt);
    std::string line;

    int row = 3;

    while (std::getline(stream, line) &&
           row < height - 4)
    {
        mvwprintw(win,
                  row,
                  2,
                  "%s",
                  line.c_str());

        row++;
    }

    mvwprintw(win,
              height - 3,
              2,
              "> ");

    wmove(win,
          height - 3,
          4);

    echo();
    curs_set(1);

    char buffer[128];

    wgetnstr(win,
             buffer,
             sizeof(buffer) - 1);

    noecho();
    curs_set(0);

    result = buffer;

    delwin(win);

    return true;
}

bool confirm_yes(const std::string& message)
{
    int width = std::min(72, COLS - 4);

    if (width < 40)
        width = 40;

    std::vector<std::string> lines;

    std::istringstream input(message);
    std::string paragraph;

    while (std::getline(input, paragraph))
    {
        if (paragraph.empty())
        {
            lines.push_back("");
            continue;
        }

        while (static_cast<int>(paragraph.size()) > width - 6)
        {
            size_t break_pos =
                paragraph.rfind(' ', width - 6);

            if (break_pos == std::string::npos)
                break_pos = width - 6;

            lines.push_back(paragraph.substr(0, break_pos));

            paragraph.erase(0, break_pos);

            while (!paragraph.empty() && paragraph[0] == ' ')
                paragraph.erase(0, 1);
        }

        lines.push_back(paragraph);
    }

    // Extra space for:
    // title
    // message
    // input
    // bottom border
    int height = static_cast<int>(lines.size()) + 5;

    if (height > LINES - 2)
        height = LINES - 2;

    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;

    WINDOW* win = newwin(height, width, start_y, start_x);

    if (!win)
        return false;

    keypad(win, TRUE);

    box(win, 0, 0);

    mvwprintw(win, 1, 2, "Confirmation");

    int row = 3;

    for (const std::string& line : lines)
    {
        if (row >= height - 2)
            break;

        mvwprintw(win, row, 2, "%s", line.c_str());
        row++;
    }

    // Input line is always above the bottom border.
    int input_row = height - 2;

    mvwprintw(win, input_row, 2, "> ");
    wmove(win, input_row, 4);

    wrefresh(win);

    echo();
    curs_set(1);

    char buffer[32] = {};

    wgetnstr(win, buffer, sizeof(buffer) - 1);

    noecho();
    curs_set(0);

    std::string answer(buffer);

    delwin(win);

    // Case-insensitive confirmation.
    for (char& c : answer)
    {
        c = static_cast<char>(
            std::tolower(static_cast<unsigned char>(c))
        );
    }

    return answer == "yes" || answer == "y";
}

partition_table_type choose_partition_table()
{
    int height = 9;
    int width = 50;

    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;

    WINDOW* win =
        newwin(height, width, start_y, start_x);

    if (!win)
        return partition_table_type::CANCEL;

    keypad(win, true);

    int highlight = 0;

    const char* options[] =
    {
        "GPT",
        "MBR",
        "Cancel"
    };

    while (true)
    {
        werase(win);
        box(win, 0, 0);

        mvwprintw(win,
                  1,
                  2,
                  "Create partition table");

        mvwprintw(win,
                  3,
                  2,
                  "Select partition table type:");

        for (int i = 0; i < 3; ++i)
        {
            if (i == highlight)
                wattron(win, A_REVERSE);

            mvwprintw(win,
                      5 + i,
                      4,
                      "%s",
                      options[i]);

            if (i == highlight)
                wattroff(win, A_REVERSE);
        }

        wrefresh(win);

        int ch = wgetch(win);

        if (ch == KEY_UP)
        {
            highlight--;

            if (highlight < 0)
                highlight = 2;
        }
        else if (ch == KEY_DOWN)
        {
            highlight++;

            if (highlight > 2)
                highlight = 0;
        }
        else if (ch == '\n' || ch == KEY_ENTER)
        {
            partition_table_type result;

            if (highlight == 0)
                result = partition_table_type::GPT;
            else if (highlight == 1)
                result = partition_table_type::MBR;
            else
                result = partition_table_type::CANCEL;

            delwin(win);

            return result;
        }
        else if (ch == 27)
        {
            delwin(win);
            return partition_table_type::CANCEL;
        }
    }
}

filesystem_type choose_filesystem()
{
    int height = 12;
    int width = 50;

    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;

    WINDOW* win =
        newwin(height, width, start_y, start_x);

    if (!win)
        return filesystem_type::CANCEL;

    keypad(win, true);

    int highlight = 0;

    const char* options[] =
    {
        "ext4",
        "xfs",
        "btrfs",
        "vfat",
        "swap",
        "Cancel"
    };

    while (true)
    {
        werase(win);
        box(win, 0, 0);

        mvwprintw(
            win,
            1,
            2,
            "Format partition"
        );

        mvwprintw(
            win,
            3,
            2,
            "Select filesystem:"
        );

        for (int i = 0; i < 6; ++i)
        {
            if (i == highlight)
                wattron(win, A_REVERSE);

            mvwprintw(
                win,
                5 + i,
                4,
                "%s",
                options[i]
            );

            if (i == highlight)
                wattroff(win, A_REVERSE);
        }

        wrefresh(win);

        int ch = wgetch(win);

        if (ch == KEY_UP)
        {
            --highlight;

            if (highlight < 0)
                highlight = 5;
        }
        else if (ch == KEY_DOWN)
        {
            ++highlight;

            if (highlight > 5)
                highlight = 0;
        }
        else if (ch == '\n' || ch == KEY_ENTER)
        {
            filesystem_type result;

            switch (highlight)
            {
                case 0:
                    result = filesystem_type::EXT4;
                    break;

                case 1:
                    result = filesystem_type::XFS;
                    break;

                case 2:
                    result = filesystem_type::BTRFS;
                    break;

                case 3:
                    result = filesystem_type::VFAT;
                    break;

                case 4:
                    result = filesystem_type::SWAP;
                    break;

                default:
                    result = filesystem_type::CANCEL;
                    break;
            }

            delwin(win);
            return result;
        }
        else if (ch == 27)
        {
            delwin(win);
            return filesystem_type::CANCEL;
        }
    }
}

bool confirm_create_partition_table(
    const std::string& device,
    const std::string& type)
{
    std::ostringstream message;

    message << device
            << ": create a new "
            << type
            << " partition table?\n\n"
            << "This is required before partitions can be created.\n"
            << "Existing partition-table information will be replaced.\n"
            << "Any existing partitions may become inaccessible.\n\n"
            << "Type yes to continue:";

    return confirm_yes(message.str());
}

std::string read_file(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        return "";

    std::string value;
    std::getline(file, value);

    return value;
}

bool sysfs_exists(const std::string& path)
{
    std::ifstream file(path);
    return file.good();
}

std::vector<block_device> get_devices()
{
    std::ifstream file("/proc/partitions");

    if (!file.is_open())
        die("Failed to open /proc/partitions");

    std::string line;
    std::vector<block_device> devices;

    // Skip headers.
    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {

        std::stringstream ss(line);

        unsigned long major;
        unsigned long minor;
        unsigned long long blocks;
        std::string name;

        if (!(ss >> major >> minor >> blocks >> name))
            continue;

        // Linux tells us whether this is a partition.
        bool is_partition =
            sysfs_exists("/sys/class/block/" + name + "/partition");

        devices.push_back({
            name,
            major,
            minor,
            blocks,
            is_partition
        });
    }

    return devices;
}

std::string trim(const std::string& str)
{
    size_t first = str.find_first_not_of(" \t\n\r");

    if (first == std::string::npos)
        return "";

    size_t last = str.find_last_not_of(" \t\n\r");

    return str.substr(first, last - first + 1);
}

bool is_number(const std::string& s)
{
    if (s.empty())
        return false;

    for (char c : s)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
    }

    return true;
}

bool run_command(const std::string& command)
{
    int result = std::system(command.c_str());

    return result == 0;
}

bool find_free_space(const std::string& device,
                     double& free_start,
                     double& free_end,
                     std::string& error)
{
    std::string command =
        "parted -s " + device +
        " unit MiB print free 2>&1";

    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
    {
        error = "Could not start parted.";
        return false;
    }

    char buffer[512];
    std::string output;

    while (fgets(buffer, sizeof(buffer), pipe))
        output += buffer;

    int status = pclose(pipe);

    if (status != 0)
    {
        error = trim(output);

        if (error.empty())
            error = "parted failed.";

        return false;
    }

    std::istringstream stream(output);
    std::string line;

    bool found = false;

    double best_start = 0.0;
    double best_end = 0.0;
    double best_size = 0.0;

    while (std::getline(stream, line))
    {
        std::istringstream ss(line);

        std::vector<std::string> fields;
        std::string field;

        while (ss >> field)
            fields.push_back(field);

        if (fields.size() < 2)
            continue;

        if (fields.size() < 2 ||
            fields[fields.size() - 2] != "Free" ||
            fields[fields.size() - 1] != "Space")
        {
            continue;
        }

        try
        {
            std::string start_text = fields[0];
            std::string end_text = fields[1];

            size_t start_pos =
                start_text.find("MiB");

            size_t end_pos =
                end_text.find("MiB");

            if (start_pos == std::string::npos ||
                end_pos == std::string::npos)
            {
                continue;
            }

            start_text.erase(start_pos);
            end_text.erase(end_pos);

            double start =
                std::stod(start_text);

            double end =
                std::stod(end_text);

            /*
             * Ignore tiny regions before the first
             * usable MiB boundary.
             */
            if (start < 1.0)
                start = 1.0;

            double size = end - start;

            if (size <= 0.0)
                continue;

            /*
             * Keep the largest usable free region.
             *
             * This avoids accidentally selecting the tiny
             * alignment area at the beginning of a GPT disk.
             */
            if (!found || size > best_size)
            {
                found = true;

                best_start = start;
                best_end = end;
                best_size = size;
            }
        }
        catch (...)
        {
            continue;
        }
    }

    if (!found)
    {
        error = "No usable free space was found.";
        return false;
    }

    free_start = best_start;
    free_end = best_end;

    return true;
}

bool confirm_delete(const std::string& device)
{
    std::ostringstream message;

    message << device
            << ": delete partition?\n\n"
            << "WARNING: This will remove the partition\n"
            << "from the partition table.\n"
            << "Data on this partition may become inaccessible.\n\n"
            << "Type 'yes' to continue:";

    return confirm_yes(message.str());
}

bool format_partition(const block_device& dev)
{
    if (!dev.is_partition)
    {
        show_message(
            "Format partition",
            "Select a partition, not a disk."
        );

        return false;
    }

    std::string device =
        "/dev/" + dev.name;

    filesystem_type choice =
        choose_filesystem();

    if (choice == filesystem_type::CANCEL)
        return false;

    std::string filesystem;
    std::string command_name;

    switch (choice)
    {
        case filesystem_type::EXT4:
            filesystem = "ext4";
            command_name = "mkfs.ext4";
            break;

        case filesystem_type::XFS:
            filesystem = "xfs";
            command_name = "mkfs.xfs -f";
            break;

        case filesystem_type::BTRFS:
            filesystem = "btrfs";
            command_name = "mkfs.btrfs -f";
            break;

        case filesystem_type::VFAT:
            filesystem = "vfat";
            command_name = "mkfs.vfat";
            break;

        case filesystem_type::SWAP:
            filesystem = "swap";
            command_name = "mkswap";
            break;

        default:
            return false;
    }

    std::ostringstream message;

    message << device
            << ": format as "
            << filesystem
            << "?\n\n"
            << "WARNING: Formatting will destroy\n"
            << "existing data on this partition.\n\n"
            << "Type yes to continue:";

    if (!confirm_yes(message.str()))
        return false;

    std::ostringstream command;

    command << command_name
            << " "
            << device
            << " 2>&1";

    FILE* pipe =
        popen(command.str().c_str(), "r");

    if (!pipe)
    {
        show_message(
            "Format partition",
            "Could not start formatting command."
        );

        return false;
    }

    char buffer[512];
    std::string output;

    while (fgets(buffer, sizeof(buffer), pipe))
        output += buffer;

    int status = pclose(pipe);

    if (status != 0)
    {
        if (output.empty())
            output = "Formatting failed.";

        show_message(
            "Format partition",
            trim(output)
        );

        return false;
    }

    sync();

    show_message(
        "Format partition",
        device + " was formatted as " + filesystem + "."
    );

    return true;
}

std::string get_partition_table_type(const std::string& device)
{
    std::string command =
        "lsblk -dn -o PTTYPE " +
        device +
        " 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");

    if (!pipe)
        return "";

    char buffer[128];

    if (!fgets(buffer, sizeof(buffer), pipe))
    {
        pclose(pipe);
        return "";
    }

    pclose(pipe);

    std::string type = trim(buffer);

    return type;
}

bool create_partition_table(const block_device& dev)
{
    if (dev.is_partition)
    {
        show_message(
            "Create partition table",
            "Select a disk, not a partition.");

        return false;
    }

    std::string device = "/dev/" + dev.name;

    partition_table_type choice =
        choose_partition_table();

    if (choice == partition_table_type::CANCEL)
        return false;

    std::string type;
    std::string label;

    if (choice == partition_table_type::GPT)
    {
        type = "GPT";
        label = "gpt";
    }
    else
    {
        type = "MBR";
        label = "msdos";
    }

    if (!confirm_create_partition_table(
            device,
            type))
    {
        return false;
    }

    std::ostringstream command;

    command << "parted -s "
            << device
            << " mklabel "
            << label
            << " 2>&1";

    FILE* pipe = popen(command.str().c_str(), "r");

    if (!pipe)
    {
        show_message(
            "Create partition table",
            "Could not start parted.");

        return false;
    }

    char buffer[512];
    std::string output;

    while (fgets(buffer, sizeof(buffer), pipe))
        output += buffer;

    int status = pclose(pipe);

    if (status != 0)
    {
        if (output.empty())
            output = "parted failed.";

        show_message(
            "Create partition table",
            trim(output));

        return false;
    }

    sync();

    return true;
}

bool create_partition(const block_device& dev)
{
    if (dev.is_partition)
    {
        show_message(
            "Create partition",
            "Select a disk, not a partition.");

        return false;
    }

    std::string device =
        "/dev/" + dev.name;

    /*
     * Check whether a partition table exists.
     */
    std::string partition_table =
        get_partition_table_type(device);

    /*
     * No partition table.
     *
     * Ask whether the user wants to create one.
     */
    if (partition_table.empty())
    {
        std::ostringstream message;

        message << device
                << " has no partition table.\n\n"
                << "Create a partition table now?\n"
                << "A GPT partition table will be used.";

        if (!confirm_yes(message.str()))
            return false;

        /*
         * Create GPT directly here.
         *
         * The explicit 't' command can still be used
         * when the user wants to choose GPT or MBR.
         */
        std::ostringstream command;

        command << "parted -s "
                << device
                << " mklabel gpt 2>&1";

        FILE* pipe =
            popen(command.str().c_str(), "r");

        if (!pipe)
        {
            show_message(
                "Create partition",
                "Could not start parted.");

            return false;
        }

        char buffer[512];
        std::string output;

        while (fgets(buffer, sizeof(buffer), pipe))
            output += buffer;

        int status = pclose(pipe);

        if (status != 0)
        {
            if (output.empty())
                output = "parted failed.";

            show_message(
                "Create partition",
                trim(output));

            return false;
        }

        sync();
    }

    /*
     * Find free space.
     */
    double free_start = 0.0;
    double free_end = 0.0;

    std::string error;

    if (!find_free_space(
            device,
            free_start,
            free_end,
            error))
    {
        show_message(
            "Create partition",
            error);

        return false;
    }

    double free_size =
        free_end - free_start;

    if (free_size < 1.0)
    {
        show_message(
            "Create partition",
            "There is not enough free space.");

        return false;
    }

    /*
     * Ask for partition size.
     */
    std::ostringstream prompt;

    prompt << "Free space: "
           << static_cast<unsigned long long>(free_size)
           << " MiB\n"
           << "Enter partition size in MiB:";

    std::string input;

    if (!input_dialog(
            "Create partition",
            prompt.str(),
            input))
    {
        return false;
    }

    if (!is_number(input))
    {
        show_message(
            "Create partition",
            "Invalid partition size.");

        return false;
    }

    unsigned long long requested_size =
        std::strtoull(
            input.c_str(),
            nullptr,
            10);

    if (requested_size == 0)
    {
        show_message(
            "Create partition",
            "Partition size must be greater than zero.");

        return false;
    }

    if (static_cast<double>(requested_size) >
        free_size)
    {
        show_message(
            "Create partition",
            "Requested size is larger than available space.");

        return false;
    }

    double start = free_start;
    double end =
        start + static_cast<double>(requested_size);

    /*
     * Create the partition.
     *
     * Don't throw away parted's error output.
     */
    std::ostringstream command;

    command << "parted -s "
        << device
        << " mkpart primary "
        << start
        << "MiB "
        << end
        << "MiB 2>&1";

    FILE* pipe =
        popen(command.str().c_str(), "r");

    if (!pipe)
    {
        show_message(
            "Create partition",
            "Could not start parted.");

        return false;
    }

    char buffer[512];
    std::string output;

    while (fgets(buffer, sizeof(buffer), pipe))
        output += buffer;

    int status = pclose(pipe);

    if (status != 0)
    {
        if (output.empty())
            output = "parted failed to create the partition.";

        show_message(
            "Create partition",
            trim(output));

        return false;
    }

    sync();

    return true;
}

bool delete_partition(const block_device& dev)
{
    if (!dev.is_partition)
        return false;

    std::string device = "/dev/" + dev.name;

    if (!confirm_delete(device))
        return false;

    /*
     * Extract the partition number.
     *
     * Examples:
     *   sda1     -> 1
     *   sda2     -> 2
     *   nvme0n1p1 -> 1
     *   nvme0n1p2 -> 2
     */
    std::string partition_number;

    for (int i = static_cast<int>(dev.name.size()) - 1;
         i >= 0;
         --i)
    {
        if (std::isdigit(
                static_cast<unsigned char>(dev.name[i])))
        {
            partition_number.insert(
                partition_number.begin(),
                dev.name[i]);
        }
        else
        {
            break;
        }
    }

    if (partition_number.empty())
        return false;

    std::string parent = dev.name;

    /*
     * For NVMe/MMC-style names:
     *
     * nvme0n1p2 -> nvme0n1
     * mmcblk0p1 -> mmcblk0
     *
     * For normal names:
     *
     * sda2 -> sda
     */
    if (!partition_number.empty())
    {
        size_t number_start =
            dev.name.size() - partition_number.size();

        if (number_start > 0 &&
            dev.name[number_start - 1] == 'p')
        {
            parent =
                dev.name.substr(0, number_start - 1);
        }
        else
        {
            parent =
                dev.name.substr(0, number_start);
        }
    }

    std::string parent_device = "/dev/" + parent;

    std::ostringstream command;

    command << "parted -s "
            << parent_device
            << " rm "
            << partition_number
            << " >/dev/null 2>&1";

    return run_command(command.str());
}

device_info get_device_info(const block_device& dev)
{
    device_info info;

    info.name = dev.name;
    info.path = "/dev/" + dev.name;

    std::string base = "/sys/class/block/" + dev.name;

    info.read_only =
        read_file(base + "/ro") == "1";

    info.removable =
        read_file(base + "/removable") == "1";

    /*
     * lsblk gives us filesystem, UUID, mountpoint,
     * model, size, etc.
     */

    std::string command =
        "lsblk -dn -o "
        "TYPE,SIZE,FSTYPE,LABEL,UUID,MOUNTPOINTS,MODEL,SERIAL "
        "/dev/" + dev.name + " 2>/dev/null";

    FILE* pipe = popen(command.c_str(), "r");

    if (pipe) {

        char buffer[1024];

        if (fgets(buffer, sizeof(buffer), pipe)) {

            std::stringstream ss(buffer);

            std::string type;
            std::string size;
            std::string filesystem;
            std::string label;
            std::string uuid;
            std::string mountpoint;
            std::string model;
            std::string serial;

            ss >> type
               >> size
               >> filesystem
               >> label
               >> uuid
               >> mountpoint
               >> model
               >> serial;

            info.type = type;
            info.size = size;
            info.filesystem = filesystem;
            info.label = label;
            info.uuid = uuid;
            info.mountpoint = mountpoint;
            info.model = model;
            info.serial = serial;
        }

        pclose(pipe);
    }

    /*
     * If lsblk didn't give us a type, derive it ourselves.
     */
    if (info.type.empty())
        info.type = dev.is_partition ? "part" : "disk";

    return info;
}

void print_field(
    WINDOW* win,
    int row,
    const char* label,
    const std::string& value)
{
    mvwprintw(
        win,
        row,
        2,
        "%-14s %s",
        label,
        value.empty() ? "N/A" : value.c_str()
    );
}

void draw_preview(
    WINDOW* win,
    const block_device& dev)
{
    werase(win);
    box(win, 0, 0);

    device_info info = get_device_info(dev);

    mvwprintw(
        win,
        0,
        2,
        " Device information "
    );

    print_field(win, 2, "Path:", info.path);
    print_field(win, 3, "Name:", info.name);
    print_field(win, 4, "Type:", info.type);
    print_field(win, 5, "Size:", info.size);

    print_field(win, 7, "Filesystem:", info.filesystem);
    print_field(win, 8, "Label:", info.label);
    print_field(win, 9, "UUID:", info.uuid);
    print_field(win, 10, "Mountpoint:", info.mountpoint);

    print_field(win, 12, "Model:", info.model);
    print_field(win, 13, "Serial:", info.serial);

    print_field(
        win,
        15,
        "Read-only:",
        info.read_only ? "yes" : "no"
    );

    print_field(
        win,
        16,
        "Removable:",
        info.removable ? "yes" : "no"
    );

    wrefresh(win);
}

void draw_device_list(
    WINDOW* win,
    const std::vector<block_device>& devices,
    int highlight
)
{
    werase(win);
    box(win, 0, 0);

    mvwprintw(win, 0, 2, " Devices ");

    int row = 1;

    for (size_t i = 0; i < devices.size(); ++i) {
        const auto& dev = devices[i];

        if (static_cast<int>(i) == highlight)
            wattron(win, A_REVERSE);

        if (dev.is_partition) {
            mvwprintw(
                win,
                row,
                2,
                "    [Part] %-10s %8llu MB",
                dev.name.c_str(),
                dev.blocks / 2048
            );
        } else {
            mvwprintw(
                win,
                row,
                2,
                "[Disk] %-10s %8llu MB",
                dev.name.c_str(),
                dev.blocks / 2048
            );
        }

        if (static_cast<int>(i) == highlight)
            wattroff(win, A_REVERSE);

        ++row;
    }

    wrefresh(win);
}

void show_details(const block_device& dev)
{
    device_info info = get_device_info(dev);

    /*
     * We build a list of lines instead of directly
     * drawing everything. That makes scrolling easy.
     */
    std::vector<std::string> lines;

    lines.push_back("Device information");
    lines.push_back("------------------");
    lines.push_back("");

    lines.push_back("Path:        " + info.path);
    lines.push_back("Name:        " + info.name);
    lines.push_back("Type:        " + info.type);
    lines.push_back("Size:        " + info.size);

    lines.push_back("");

    lines.push_back("Filesystem:  " + info.filesystem);
    lines.push_back("Label:       " + info.label);
    lines.push_back("UUID:        " + info.uuid);
    lines.push_back("Mountpoint:  " + info.mountpoint);

    lines.push_back("");

    lines.push_back("Model:       " + info.model);
    lines.push_back("Serial:      " + info.serial);

    lines.push_back("");

    lines.push_back(
        "Read-only:   " +
        std::string(info.read_only ? "yes" : "no")
    );

    lines.push_back(
        "Removable:   " +
        std::string(info.removable ? "yes" : "no")
    );

    int scroll = 0;

    while (true) {

        werase(stdscr);
        box(stdscr, 0, 0);

        mvprintw(
            0,
            2,
            " %s ",
            info.path.c_str()
        );

        int height = getmaxy(stdscr);
        int width = getmaxx(stdscr);

        int visible_lines = height - 4;

        for (int i = 0; i < visible_lines; ++i) {

            int line_index = scroll + i;

            if (line_index >= static_cast<int>(lines.size()))
                break;

            mvprintw(
                i + 2,
                2,
                "%.*s",
                width - 4,
                lines[line_index].c_str()
            );
        }

        mvprintw(
            height - 2,
            2,
            "↑↓ Scroll   Esc Back"
        );

        refresh();

        int ch = getch();

        switch (ch) {

            case KEY_UP:

                if (scroll > 0)
                    --scroll;

                break;

            case KEY_DOWN:

                if (scroll + visible_lines <
                    static_cast<int>(lines.size()))
                    ++scroll;

                break;

            case 27: // ESC

                return;
        }
    }
}

void draw_keybindings(WINDOW* win)
{
    werase(win);
    box(win, 0, 0);

    mvwprintw(
        win,
        1,
        2,
        "↑↓ Select   Enter Details   c Create   t Table"
    );

    mvwprintw(
        win,
        2,
        2,
        "f Format   d Delete   q Quit"
    );

    wrefresh(win);
}

int main()
{
    initscr();

    noecho();
    curs_set(0);
    keypad(stdscr, true);

    if (has_colors()) {
        start_color();
    }

    auto devices = get_devices();

    if (devices.empty()) {
        endwin();

        std::fprintf(
            stderr,
            "No block devices found.\n"
        );

        return EXIT_FAILURE;
    }

    int highlight = 0;

    int height;
    int width;

    getmaxyx(stdscr, height, width);

    /*
     * Left pane = roughly 40% of screen.
     * Right pane = remaining 60%.
     */
    int left_width = width * 40 / 100;
    int right_width = width - left_width;

   int key_height = 4;
    int pane_height = height - key_height;

    WINDOW* list_win =
        newwin(
            pane_height,
            left_width,
            0,
            0
        );

    WINDOW* info_win =
        newwin(
            pane_height,
            right_width,
            0,
            left_width
        );

    WINDOW* key_win =
        newwin(
            key_height,
            width,
            pane_height,
            0
        );

    keypad(list_win, true);

    bool running = true;

    while (running) {

        draw_device_list(
            list_win,
            devices,
            highlight
        );

        draw_preview(
            info_win,
            devices[highlight]
        );

        draw_keybindings(key_win);

        int ch = wgetch(list_win);

        switch (ch) {

            case KEY_UP:

                --highlight;

                if (highlight < 0)
                    highlight =
                        static_cast<int>(devices.size()) - 1;

                break;

            case KEY_DOWN:

                ++highlight;

                if (highlight >=
                    static_cast<int>(devices.size()))
                    highlight = 0;

                break;

            case '\n':
            case KEY_ENTER:

                show_details(
                    devices[highlight]
                );

                break;

            case 'q':
            case 'Q':

                running = false;

                break;
            
            case 'f':
            case 'F':
            {
                const block_device& dev =
                    devices[highlight];

                if (!dev.is_partition)
                {
                    show_message(
                        "Format partition",
                        "Select a partition, not a disk."
                    );

                    break;
                }

                if (geteuid() != 0)
                {
                    show_message(
                        "Format partition",
                        "Root privileges are required."
                    );

                    break;
                }

                if (format_partition(dev))
                {
                    devices = get_devices();

                    if (devices.empty())
                        break;

                    if (highlight >=
                        static_cast<int>(devices.size()))
                    {
                        highlight =
                            static_cast<int>(devices.size()) - 1;
                    }
                }

                break;
            }
                
            case 't':
            case 'T':
            {
                const block_device& dev =
                    devices[highlight];

                if (dev.is_partition)
                {
                    show_message(
                        "Create partition table",
                        "Select a disk, not a partition.");

                    break;
                }

                if (geteuid() != 0)
                {
                    show_message(
                        "Create partition table",
                        "Root privileges are required.");

                    break;
                }

                if (create_partition_table(dev))
                {
                    devices = get_devices();

                    if (devices.empty())
                        break;

                    if (highlight >=
                        static_cast<int>(devices.size()))
                    {
                        highlight =
                            static_cast<int>(devices.size()) - 1;
                    }
                }
                else
                {
                    show_message(
                        "Create partition table",
                        "Partition table was not created.");
                }

                break;
            }
            
            case 'c':
            case 'C':
            {
                const block_device& dev =
                    devices[highlight];

                if (dev.is_partition)
                {
                    show_message(
                        "Create partition",
                        "Select a disk, not a partition.");

                    break;
                }

                if (geteuid() != 0)
                {
                    show_message(
                        "Create partition",
                        "Root privileges are required.");

                    break;
                }

                if (create_partition(dev))
                {
                    devices = get_devices();

                    if (devices.empty())
                        break;

                    if (highlight >=
                        static_cast<int>(devices.size()))
                    {
                        highlight =
                            static_cast<int>(devices.size()) - 1;
                    }
                }

                break;
            }

            case 'd':
            case 'D':
            {
                const block_device& dev = devices[highlight];

                if (!dev.is_partition)
                {
                    show_message(
                        "Delete partition",
                        "Select a partition, not a disk.");
                    break;
                }

                if (geteuid() != 0)
                {
                    show_message(
                        "Delete partition",
                        "Root privileges are required.");
                    break;
                }

                if (delete_partition(dev))
                {
                    devices = get_devices();

                    if (devices.empty())
                        break;

                    if (highlight >= static_cast<int>(devices.size()))
                        highlight = static_cast<int>(devices.size()) - 1;
                }
                else
                {
                    show_message(
                        "Delete partition",
                        "Partition was not deleted.");
                }

                break;
            }
        }
    }

    delwin(list_win);
    delwin(info_win);
    delwin(key_win);

    endwin();

    return 0;
}