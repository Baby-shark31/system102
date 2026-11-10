#include <stdio.h>

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"

int main() {
    
    printf(RED"============================\n"RESET);
    printf(RED"||" YELLOW "  WEIGHT CONVERSION     " RED "||\n"RESET);
    printf(RED"============================\n"RESET);
    printf("1. Kilograms to Pounds\n");
    printf("2. Pounds to Kilograms\n");
    printf("Select an option (1 or 2): ");

    int choice;
    scanf("%d", &choice);

    float kilograms, pounds;

    if(choice == 1) {
        printf("Enter weight in kilograms: ");
        scanf("%f", &kilograms);
        pounds = kilograms * 2.20462;
        printf("%.2f kilograms is equal to %.2f pounds.\n", kilograms, pounds);
    } else if(choice == 2) {
        printf("Enter weight in pounds: ");
        scanf("%f", &pounds);
        kilograms = pounds / 2.20462;
        printf("%.2f pounds is equal to %.2f kilograms.\n", pounds, kilograms);
    } else {
        printf(RED"Invalid choice. Please select 1 or 2.\n"RESET);

    }

    return 0;
}