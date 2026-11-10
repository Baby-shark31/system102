#include <stdio.h>

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1b[37m"

int main()
{

    printf(BLUE "==========================\n" RESET);
    printf(YELLOW " PERSONAL INFORMATION \n" RESET);
    printf(RED "==========================\n" RESET);
    
    int age;
    char name[50];
    double Gwa;
    char grade;


    printf("Enter your name: ");
    fgets(name, sizeof(name), stdin);
    printf("Enter your age: ");
    scanf("%d", &age);
    printf("Enter your GWA: ");
    scanf("%lf", &Gwa);
    printf("Enter your grade: ");
    scanf(" %c", &grade);


    printf(BLUE"Your name is: " GREEN "%s\n" RESET, name);
    printf(RED"Your age is: " RED "%d\n" RESET, age);
    printf(YELLOW"Your GWA is: " BLUE "%.2lf\n" RESET, Gwa);
    printf(GREEN "Your grade is: " YELLOW "%c\n" RESET, grade);


     


    return(0);
}