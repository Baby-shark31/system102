#include <stdio.h>
#include <string.h>

struct Student

{
    char name[50];
    float gpa;
};


int main() {
   
    struct Student student1 = {"Daren", 1.75};
    struct Student student2 = {"Kaiel", 2.00};
    struct Student student3 = {"Christian", 2.00};
    struct Student student4 = {"Jebhie", 1.50};
    struct Student student5 = {"Mateo", 1.25};
    struct Student student6 = {"Yexiel", 1.75};
    struct Student student7 = {"Zillian", 1.25};
    struct Student student8 = {"Jairuze", 1.50};

    struct Student students[] = {student1, student2, student3, student4, student5, student6, student7, student8};

    for (int i = 0; i < sizeof(students) / sizeof(students[0]); i++) {
        printf("%-12s\t", students[i].name);
        printf("%.2f\n", students[i].gpa);
        
    }

    

    
    
}