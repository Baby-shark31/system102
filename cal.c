#include <stdio.h>
#include <stdlib.h>
#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"

int main() {
    
    int choice;
    double num1, num2, result;
    char again;

    do{
        printf(BLUE "+---------------------------+\n" RESET);
        printf(BLUE "| " YELLOW "Simple Calculator" BLUE "         |\n" RESET);
        printf(BLUE "+---------------------------+\n" RESET);
        printf(BLUE "| " YELLOW "1. Addition" BLUE "               |\n" RESET);
        printf(BLUE "| " YELLOW "2. Subtraction" BLUE "            |\n" RESET);
        printf(BLUE "| " YELLOW "3. Multiplication" BLUE "         |\n" RESET);
        printf(BLUE "| " YELLOW "4. Division" BLUE "               |\n" RESET);
        printf(BLUE "+---------------------------+\n" RESET);   

        printf("Select an operation (1-4): ");
        scanf("%d", &choice);

        if(choice < 1 || choice > 4) {
            printf("Invalid choice!\n");
            continue;
        }

        printf("Enter first number: ");
        scanf("%lf", &num1);
        printf("Enter second number: ");
        scanf("%lf", &num2);

        switch(choice) {
            case 1:
                result = num1 + num2;
                printf("Result: %.2lf + %.2lf = %.2lf\n", num1, num2, result);
                break;
            case 2:
                result = num1 - num2;
                printf("Result: %.2lf - %.2lf = %.2lf\n", num1, num2, result);
                break;
            case 3:
                result = num1 * num2;
                printf("Result: %.2lf * %.2lf = %.2lf\n", num1, num2, result);
                break;
            case 4:
                if(num2 != 0) {
                    result = num1 / num2;
                    printf("Result: %.2lf / %.2lf = %.2lf\n", num1, num2, result);
                } else {
                    printf("Error: Division by zero is not allowed.\n");
                    system("cls"); // Clear the console
                }

                break;
        }
        printf("Do you want to perform another calculation? (y/n): ");
        scanf(" %c", &again);
        
    } while(again == 'y' || again == 'Y');
    printf("Thank you for using the calculator!\n");
    return 0;
}