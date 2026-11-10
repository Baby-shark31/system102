#include <stdio.h>

// Define ANSI Escape Codes for Colors
#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"



int main() {
    // Table Header
    printf(BLUE "+----+------------+-------+\n" RESET);
    printf(BLUE "| " YELLOW "#" BLUE "  | " YELLOW "Sell   " BLUE "    | " YELLOW "Price" BLUE " |\n" RESET);
    printf(BLUE "+----+------------+-------+\n" RESET);

    // Table Rows with styled text content
    printf(BLUE "| " WHITE "01" BLUE " | " CYAN "Keychains " BLUE " | " GREEN "$1200" BLUE " |\n" RESET);
    printf(BLUE "| " WHITE "02" BLUE " | " CYAN "Ballpen   " BLUE " | " GREEN "$  25" BLUE " |\n" RESET);
    printf(BLUE "| " WHITE "03" BLUE " | " CYAN "Calendar  " BLUE " | " GREEN "$  80" BLUE " |\n" RESET);

    // Table Footer
    printf(BLUE "+----+------------+-------+\n" RESET);

    return 0;
}