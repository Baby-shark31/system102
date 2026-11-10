#include <stdio.h>
#define RESET  "\x1b[0m"
#define RED    "\x1b[31m"
#define YELLOW "\x1b[33m"
#define BLUE  "\x1b[34m"

int main() {
    char choice = '0';
    float fahrenheit = 0.0f;
    float celsius = 0.0f;

    printf(BLUE "=================================\n" RESET);
    printf(BLUE"===" YELLOW "   TEMPERATURE CONVERSION  " BLUE "===\n" RESET);
    printf(BLUE "=================================\n" RESET);
    printf("C. Celsius to Fahrenheit\n");
    printf("F. Fahrenheit to Celsius\n");
    printf("Is the temperature in Celsius (C) or Fahrenheit (F)? ");
    scanf(" %c", &choice);

    if (choice == 'C' || choice == 'c') {
        printf("Enter temperature in Celsius: ");
        scanf("%f", &celsius);
        fahrenheit = (celsius * 9 / 5) + 32;
        printf("%.1f Celcius is equal to %.1f Fahrenheit.\n", celsius, fahrenheit);
        
    }
    else if (choice == 'F' || choice == 'f') {
        printf("Enter temperature in Fahrenheit: ");
        scanf("%f", &fahrenheit);
        celsius = (fahrenheit - 32) * 5 / 9;
        printf("%.1f Fahrenheit is equal to %.1f Celsius.\n", fahrenheit, celsius);
    }
    else {
        printf(RED "Invalid choice. Please enter 'C' or 'F'.\n" RESET);
    }
}