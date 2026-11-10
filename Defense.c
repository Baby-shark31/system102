#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif


#define USE_COLORS 1

#if USE_COLORS
#define RESET  "\033[0m"
#define BOLD   "\033[1m"
#define BLUE   "\033[1;34m"
#define YELLOW "\033[1;33m"
#define GREEN  "\033[1;32m"
#else
#define RESET  ""
#define BOLD   ""
#define BLUE   ""
#define YELLOW ""
#define GREEN  ""
#endif

#define MAX_CUSTOMERS 50
#define MAX_COMPUTERS 5
#define MAX_SALES     500

#define STAFF_USERNAME "admin"
#define STAFF_PASSWORD "admin123"
#define MAX_ATTEMPTS   5

#define HOURLY_RATE      20.0
#define DISCOUNT_PERCENT 10    

#define WALKIN -1                

#define AVAILABLE 0             
#define ASSIGNED  1
#define IN_USE    2

struct Customer {
    int id;
    char name[50];
    char contact[12];
};

struct Computer {
    int id;
    char name[10];
    int status;
    int customerId;              
    long startTime;
};

struct Sale {
    int customerId;
    int pcId;
    int minutes;
    double amount;
};

struct Customer customers[MAX_CUSTOMERS];
int customerCount = 0;

struct Computer computers[MAX_COMPUTERS];

struct Sale sales[MAX_SALES];
int saleCount = 0;
double totalSales = 0;

void loadData();
void saveData();
void enableColors();

int  staffLogin();
int  mainMenu();
void sessionsMenu();
void recordsMenu();

void newCustomer();
void registerCustomer();
void assignComputer();
void startSession();
void endSession();
void cancelAssignment();
void viewCustomers();
void editCustomer();
void deleteCustomer();
void resetCustomers();
void viewSales();

void readText(char text[], int size);
void trim(char text[]);
int  getNumber(char prompt[], int min, int max);
int  askYesNo(char prompt[]);
void getName(char name[]);
void getContact(char contact[], int selfIndex);
int  isValidName(char name[]);
int  isValidContact(char contact[]);

int  findCustomer(int id);
int  findByContact(char contact[]);
int  findCustomerPc(int customerId);
int  nextCustomerId();
char *customerName(int id);
int  billedMinutes(long startTime);
int  showAvailable();
int  chooseComputer(char action[], int wantedStatus);
int  chooseCustomer();
void assignTo(int customerId);

int main() {
    int running = 1;

    enableColors();
    loadData();

    printf(YELLOW "\n=====================================\n" RESET);
    printf(BOLD YELLOW "   COMPUTER SHOP MANAGEMENT SYSTEM \n" RESET);
    printf(YELLOW "=====================================\n" RESET);

    while (running) {
        if (staffLogin() == 0) {
            saveData();
            printf("\nSystem closed.\n");
            return 0;
        }
        running = mainMenu();  
    }

    saveData();
    printf("\nGoodbye!\n");
    return 0;
}

void enableColors() {
#ifdef _WIN32
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (GetConsoleMode(console, &mode)) {
        SetConsoleMode(console, mode | 0x0001 | 0x0002 | 0x0004);
    }
#endif
}

int staffLogin() {
    char username[50];
    char password[50];
    int attempt;

    printf(YELLOW "\n--- STAFF LOGIN ---\n" RESET);

    for (attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        printf("Username: ");
        readText(username, 50);
        printf("Password: ");
        readText(password, 50);

        if (strcmp(username, STAFF_USERNAME) == 0 &&
            strcmp(password, STAFF_PASSWORD) == 0) {
            printf(GREEN "Login successful. Welcome, Staff!\n" RESET);
            return 1;
        }

        if (attempt < MAX_ATTEMPTS) {
            printf(YELLOW "Wrong username or password! (%d attempt(s) left)\n" RESET,
                   MAX_ATTEMPTS - attempt);
        }
    }

    printf(YELLOW "Too many wrong attempts. The system will now close.\n" RESET);
    return 0;
}

int mainMenu() {
    int choice;

    while (1) {
        printf(YELLOW "\n=========== STAFF MENU ===========\n" RESET);
        printf(GREEN "1. New Customer (Register or Walk-in)\n" RESET);
        printf(GREEN "2. Computer Sessions\n" RESET);
        printf(GREEN "3. Customer Records\n" RESET);
        printf(GREEN "4. Sales Records\n" RESET);
        printf(GREEN "5. Logout\n" RESET);
        printf(GREEN "6. Exit\n" RESET);
        printf(YELLOW "----------------------------------\n" RESET);

        choice = getNumber("Enter your choice (1-6): ", 1, 6);

        if (choice == 1) {
            newCustomer();
        } else if (choice == 2) {
            sessionsMenu();
        } else if (choice == 3) {
            recordsMenu();
        } else if (choice == 4) {
            viewSales();
        } else if (choice == 5) {
            printf(BLUE "Logged out.\n" RESET);
            return 1;
        } else {
            printf(BLUE "Saving data...\n" RESET);
            return 0;
        }
    }
}

void sessionsMenu() {
    int choice;

    do {
        printf(YELLOW "\n------- COMPUTER SESSIONS -------\n" RESET);
        printf(GREEN "1. Check Available Computers\n" RESET);
        printf(GREEN "2. Assign Computer\n" RESET);
        printf(GREEN "3. Start Session\n" RESET);
        printf(GREEN "4. End Session / Calculate Payment\n" RESET);
        printf(GREEN "5. Cancel Assignment\n" RESET);
        printf(GREEN "6. Back\n" RESET);

        choice = getNumber("Enter your choice (1-6): ", 1, 6);

        if (choice == 1) {
            printf(YELLOW "\n--- AVAILABLE COMPUTERS ---\n" RESET);
            showAvailable();
        } else if (choice == 2) {
            assignComputer();
        } else if (choice == 3) {
            startSession();
        } else if (choice == 4) {
            endSession();
        } else if (choice == 5) {
            cancelAssignment();
        }
    } while (choice != 6);
}

void recordsMenu() {
    int choice;

    do {
        printf(YELLOW "\n------- CUSTOMER RECORDS -------\n" RESET);
        printf(GREEN "1. View Customers\n" RESET);
        printf(GREEN "2. Edit Customer\n" RESET);
        printf(GREEN "3. Delete Customer\n" RESET);
        printf(GREEN "4. Reset All Customers\n" RESET);
        printf(GREEN "5. Back\n" RESET);

        choice = getNumber("Enter your choice (1-5): ", 1, 5);

        if (choice == 1) {
            viewCustomers();
        } else if (choice == 2) {
            editCustomer();
        } else if (choice == 3) {
            deleteCustomer();
        } else if (choice == 4) {
            resetCustomers();
        }
    } while (choice != 5);
}



void readText(char text[], int size) {
    int ch;

    if (fgets(text, size, stdin) == NULL) {
        saveData();
        exit(0);
    }

    if (strchr(text, '\n') == NULL) {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
        text[0] = '\0';
    } else {
        text[strcspn(text, "\r\n")] = '\0';
    }
}

void trim(char text[]) {
    int start = 0;
    int end = strlen(text) - 1;
    int i;

    while (text[start] == ' ') {
        start++;
    }
    while (end >= start && text[end] == ' ') {
        end--;
    }
    for (i = 0; start + i <= end; i++) {
        text[i] = text[start + i];
    }
    text[i] = '\0';
}

int getNumber(char prompt[], int min, int max) {
    char text[20];
    int i, ok, number;

    while (1) {
        printf("%s", prompt);
        readText(text, 20);

        ok = 1;
        if (strlen(text) == 0 || strlen(text) > 6) {
            ok = 0;
        }
        for (i = 0; text[i] != '\0'; i++) {
            if (!isdigit((unsigned char)text[i])) {
                ok = 0;
            }
        }

        if (ok) {
            number = atoi(text);
            if (number >= min && number <= max) {
                return number;
            }
        }
        printf(YELLOW "Invalid input! Please enter a number from %d to %d.\n" RESET, min, max);
    }
}

int askYesNo(char prompt[]) {
    char text[10];

    while (1) {
        printf("%s (Y/N): ", prompt);
        readText(text, 10);

        if (strlen(text) == 1) {
            if (toupper((unsigned char)text[0]) == 'Y') {
                return 1;
            }
            if (toupper((unsigned char)text[0]) == 'N') {
                return 0;
            }
        }
        printf(YELLOW "Invalid input! Please type Y or N.\n" RESET);
    }
}

int isValidName(char name[]) {
    int i, letters = 0;
    unsigned char c;

    if (strlen(name) == 0) {
        return 0;
    }
    for (i = 0; name[i] != '\0'; i++) {
        c = (unsigned char)name[i];
        if (isalpha(c) || c >= 128) {
            letters++;
        } else if (c != ' ' && c != '.' && c != '-' && c != '\'') {
            return 0;
        }
    }
    return letters > 0;
}

int isValidContact(char contact[]) {
    int i;

    if (strlen(contact) != 11) {
        return 0;
    }
    for (i = 0; i < 11; i++) {
        if (!isdigit((unsigned char)contact[i])) {
            return 0;
        }
    }
    if (contact[0] != '0' || contact[1] != '9') {
        return 0;
    }
    return 1;
}

void getName(char name[]) {
    while (1) {
        printf("Enter Name: ");
        readText(name, 50);
        trim(name);

        if (isValidName(name)) {
            return;
        }
        printf(YELLOW "Invalid name! Use letters, spaces, . - ' only (max 49 characters).\n" RESET);
    }
}

void getContact(char contact[], int selfIndex) {
    char text[50];
    int found;

    while (1) {
        printf("Enter Contact No. (11 digits, starts with 09): ");
        readText(text, 50);
        trim(text);

        if (!isValidContact(text)) {
            printf(YELLOW "Invalid contact number! It must be 11 digits and start with 09.\n" RESET);
            continue;
        }

        found = findByContact(text);
        if (found != -1 && found != selfIndex) {
            printf(YELLOW "This number is already registered to %s! Enter a different number.\n" RESET,
                   customers[found].name);
            continue;
        }

        strcpy(contact, text);
        return;
    }
}

int findCustomer(int id) {
    int i;
    for (i = 0; i < customerCount; i++) {
        if (customers[i].id == id) {
            return i;
        }
    }
    return -1;
}

int findByContact(char contact[]) {
    int i;
    for (i = 0; i < customerCount; i++) {
        if (strcmp(customers[i].contact, contact) == 0) {
            return i;
        }
    }
    return -1;
}

int findCustomerPc(int customerId) {
    int i;
    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].customerId == customerId && computers[i].status != AVAILABLE) {
            return i;
        }
    }
    return -1;
}

int nextCustomerId() {
    int i, biggest = 0;

    for (i = 0; i < customerCount; i++) {
        if (customers[i].id > biggest) {
            biggest = customers[i].id;
        }
    }
    for (i = 0; i < saleCount; i++) {
        if (sales[i].customerId > biggest) {
            biggest = sales[i].customerId;
        }
    }
    return biggest + 1;
}

char *customerName(int id) {
    int index;

    if (id == WALKIN) {
        return "Walk-in Guest";
    }
    index = findCustomer(id);
    if (index == -1) {
        return "(deleted customer)";
    }
    return customers[index].name;
}

int billedMinutes(long startTime) {
    long seconds = (long)time(NULL) - startTime;
    int minutes;

    if (seconds < 0) {
        seconds = 0;
    }
    minutes = seconds / 60;
    if (seconds % 60 > 0) {
        minutes++;
    }
    if (minutes < 1) {
        minutes = 1;
    }
    return minutes;
}

int showAvailable() {
    int i, count = 0;

    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].status == AVAILABLE) {
            printf("ID: %d | Name: %s\n", computers[i].id, computers[i].name);
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No available computers right now.\n" RESET);
    }
    return count;
}

int chooseComputer(char action[], int wantedStatus) {
    int id;
    char prompt[80];

    sprintf(prompt, "Enter Computer ID %s (0 to cancel): ", action);

    while (1) {
        id = getNumber(prompt, 0, MAX_COMPUTERS);

        if (id == 0) {
            return -1;
        }
        if (computers[id - 1].status == wantedStatus) {
            return id - 1;
        }
        printf(YELLOW "Please choose one of the listed Computer IDs!\n" RESET);
    }
}

int chooseCustomer() {
    int id, index;

    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return -1;
    }

    while (1) {
        id = getNumber("Enter Customer ID (0 to cancel): ", 0, 1000000);
        if (id == 0) {
            return -1;
        }
        index = findCustomer(id);
        if (index != -1) {
            return index;
        }
        printf(YELLOW "Customer ID not found! Try again.\n" RESET);
    }
}

void registerCustomer() {
    struct Customer c;

    if (customerCount >= MAX_CUSTOMERS) {
        printf(YELLOW "The customer list is full (max %d)!\n" RESET, MAX_CUSTOMERS);
        return;
    }

    c.id = nextCustomerId();
    getName(c.name);
    getContact(c.contact, -1);

    customers[customerCount] = c;
    customerCount++;
    saveData();

    printf(GREEN "SUCCESS: %s is now a member with ID #%d!\n" RESET, c.name, c.id);
    printf(GREEN "Member privilege: %d%% off every session.\n" RESET, DISCOUNT_PERCENT);
}

void assignTo(int customerId) {
    int pc;

    if (customerId > 0 && findCustomerPc(customerId) != -1) {
        printf(YELLOW "%s already has a computer!\n" RESET, customerName(customerId));
        return;
    }

    if (showAvailable() == 0) {
        return;
    }

    pc = chooseComputer("to assign", AVAILABLE);
    if (pc == -1) {
        printf(YELLOW "Assignment cancelled.\n" RESET);
        return;
    }

    computers[pc].status = ASSIGNED;
    computers[pc].customerId = customerId;
    computers[pc].startTime = 0;
    saveData();

    printf(GREEN "SUCCESS: %s is now assigned to %s.\n" RESET, computers[pc].name, customerName(customerId));
    printf("Go to Computer Sessions > Start Session to begin.\n");
}

void newCustomer() {
    int memberId;

    printf(YELLOW "\n--- NEW CUSTOMER ---\n" RESET);
    printf("Members get %d%% off every session.\n", DISCOUNT_PERCENT);
    printf("Walk-in guests pay the regular rate and need no name or contact number.\n");

    if (askYesNo("Does the customer want to register")) {
        if (customerCount >= MAX_CUSTOMERS) {
            printf(YELLOW "The customer list is full (max %d)!\n" RESET, MAX_CUSTOMERS);
            return;
        }
        registerCustomer();
        memberId = customers[customerCount - 1].id;

        if (askYesNo("Assign a computer now")) {
            assignTo(memberId);
        }
    } else {
        printf("Continuing as a walk-in guest.\n");
        assignTo(WALKIN);
    }
}

void assignComputer() {
    int index;

    printf(YELLOW "\n--- ASSIGN COMPUTER ---\n" RESET);

    if (askYesNo("Is the customer a registered member")) {
        index = chooseCustomer();
        if (index == -1) {
            printf(YELLOW "Assignment cancelled.\n" RESET);
            return;
        }
        assignTo(customers[index].id);
    } else {
        printf("Continuing as a walk-in guest.\n");
        assignTo(WALKIN);
    }
}

void startSession() {
    int i, count = 0, pc;

    printf(YELLOW "\n--- START SESSION ---\n" RESET);

    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].status == ASSIGNED) {
            printf("ID: %d | Name: %s | Customer: %s\n",
                   computers[i].id, computers[i].name, customerName(computers[i].customerId));
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No assigned computers waiting. Assign a computer first.\n" RESET);
        return;
    }

    pc = chooseComputer("to start", ASSIGNED);
    if (pc == -1) {
        printf(YELLOW "Cancelled.\n" RESET);
        return;
    }

    computers[pc].status = IN_USE;
    computers[pc].startTime = (long)time(NULL);
    saveData();

    printf(GREEN "SUCCESS: Session started for %s on %s.\n" RESET,
           customerName(computers[pc].customerId), computers[pc].name);
}

void endSession() {
    int i, count = 0, pc, minutes, isMember;
    double subtotal, discount, payment;

    printf(YELLOW "\n--- END SESSION & CALCULATE PAYMENT ---\n" RESET);

    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].status == IN_USE) {
            printf("ID: %d | Name: %s | Customer: %s | Time so far: %d min\n",
                   computers[i].id, computers[i].name,
                   customerName(computers[i].customerId),
                   billedMinutes(computers[i].startTime));
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No computers are currently in use.\n" RESET);
        return;
    }
    if (saleCount >= MAX_SALES) {
        printf(YELLOW "Sales records are full (max %d)! Cannot end the session.\n" RESET, MAX_SALES);
        return;
    }

    pc = chooseComputer("to end", IN_USE);
    if (pc == -1) {
        printf(YELLOW "Cancelled.\n" RESET);
        return;
    }

    isMember = (computers[pc].customerId > 0);
    minutes = billedMinutes(computers[pc].startTime);
    subtotal = minutes / 60.0 * HOURLY_RATE;
    discount = 0;
    if (isMember) {
        discount = subtotal * DISCOUNT_PERCENT / 100.0;
    }
    payment = subtotal - discount;

    sales[saleCount].customerId = computers[pc].customerId;
    sales[saleCount].pcId = computers[pc].id;
    sales[saleCount].minutes = minutes;
    sales[saleCount].amount = payment;
    saleCount++;
    totalSales = totalSales + payment;

    printf(YELLOW "-------------------------------------\n" RESET);
    printf("Customer: %s\n", customerName(computers[pc].customerId));
    printf("Computer: %s\n", computers[pc].name);
    printf("Total Time: %dh %dm\n", minutes / 60, minutes % 60);
    printf("Rate: PHP %.2f / hour\n", HOURLY_RATE);
    printf("Subtotal: PHP %.2f\n", subtotal);
    if (isMember) {
        printf(GREEN "Member discount (%d%%): -PHP %.2f\n" RESET, DISCOUNT_PERCENT, discount);
    }
    printf(BOLD GREEN "TOTAL PAYMENT: PHP %.2f\n" RESET, payment);

    computers[pc].status = AVAILABLE;
    computers[pc].customerId = 0;
    computers[pc].startTime = 0;
    saveData();
}

void cancelAssignment() {
    int i, count = 0, pc;

    printf(YELLOW "\n--- CANCEL ASSIGNMENT ---\n" RESET);

    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].status == ASSIGNED) {
            printf("ID: %d | Name: %s | Customer: %s\n",
                   computers[i].id, computers[i].name, customerName(computers[i].customerId));
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No pending assignments to cancel.\n" RESET);
        return;
    }

    pc = chooseComputer("to cancel", ASSIGNED);
    if (pc == -1) {
        printf(YELLOW "Nothing cancelled.\n" RESET);
        return;
    }

    printf(GREEN "SUCCESS: Assignment of %s to %s cancelled.\n" RESET,
           computers[pc].name, customerName(computers[pc].customerId));
    computers[pc].status = AVAILABLE;
    computers[pc].customerId = 0;
    computers[pc].startTime = 0;
    saveData();
}

void viewCustomers() {
    int i, pc;

    printf(YELLOW "\n--- CUSTOMER RECORDS ---\n" RESET);
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return;
    }
    for (i = 0; i < customerCount; i++) {
        pc = findCustomerPc(customers[i].id);
        printf("ID: %d | Name: %-20s | Contact: %s | PC: ",
               customers[i].id, customers[i].name, customers[i].contact);
        if (pc == -1) {
            printf("-\n");
        } else {
            printf("%s\n", computers[pc].name);
        }
    }
    printf("Total members: %d\n", customerCount);
}

void editCustomer() {
    int i, index, choice;
    char text[50];

    printf(YELLOW "\n--- EDIT CUSTOMER ---\n" RESET);
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return;
    }
    for (i = 0; i < customerCount; i++) {
        printf("ID: %d | Name: %s\n", customers[i].id, customers[i].name);
    }

    index = chooseCustomer();
    if (index == -1) {
        return;
    }

    do {
        printf("\nEditing ID #%d | Name: %s | Contact: %s\n",
               customers[index].id, customers[index].name, customers[index].contact);
        printf(GREEN "1. Change Name\n" RESET);
        printf(GREEN "2. Change Contact No.\n" RESET);
        printf(GREEN "3. Done\n" RESET);
        choice = getNumber("Choice (1-3): ", 1, 3);

        if (choice == 1) {
            getName(text);
            strcpy(customers[index].name, text);
            saveData();
            printf(GREEN "SUCCESS: Name updated.\n" RESET);
        } else if (choice == 2) {
            getContact(text, index);
            strcpy(customers[index].contact, text);
            saveData();
            printf(GREEN "SUCCESS: Contact number updated.\n" RESET);
        }
    } while (choice != 3);
}

void deleteCustomer() {
    int i, index, pc;
    char name[50];

    printf(YELLOW "\n--- DELETE CUSTOMER ---\n" RESET);
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return;
    }
    for (i = 0; i < customerCount; i++) {
        printf("ID: %d | Name: %s\n", customers[i].id, customers[i].name);
    }

    index = chooseCustomer();
    if (index == -1) {
        return;
    }

    pc = findCustomerPc(customers[index].id);
    if (pc != -1) {
        printf(YELLOW "%s is still assigned to %s! End the session or cancel the assignment first.\n" RESET,
               customers[index].name, computers[pc].name);
        return;
    }

    if (!askYesNo("Delete this customer? This cannot be undone")) {
        printf(YELLOW "Delete cancelled.\n" RESET);
        return;
    }

    strcpy(name, customers[index].name);
    for (i = index; i < customerCount - 1; i++) {
        customers[i] = customers[i + 1];
    }
    customerCount--;
    saveData();

    printf(GREEN "SUCCESS: %s was deleted. Past sales records are kept.\n" RESET, name);
}

void resetCustomers() {
    int i, kept = 0;

    printf(YELLOW "\n--- RESET ALL CUSTOMERS ---\n" RESET);

    for (i = 0; i < MAX_COMPUTERS; i++) {
        if (computers[i].customerId > 0 && computers[i].status != AVAILABLE) {
            printf(YELLOW "%s is still assigned to a member! End the session or cancel the assignment first.\n" RESET,
                   computers[i].name);
            return;
        }
    }

    printf(YELLOW "This deletes ALL customers and all member sales. It cannot be undone.\n" RESET);
    if (!askYesNo("Are you sure")) {
        printf(YELLOW "Reset cancelled.\n" RESET);
        return;
    }

    customerCount = 0;

    totalSales = 0;
    for (i = 0; i < saleCount; i++) {
        if (sales[i].customerId == WALKIN) {
            sales[kept] = sales[i];
            totalSales = totalSales + sales[i].amount;
            kept++;
        }
    }
    saleCount = kept;
    saveData();

    printf(GREEN "SUCCESS: All customers deleted. The next member will get ID #1.\n" RESET);
}

void viewSales() {
    int i;

    printf(YELLOW "\n--- SALES RECORDS ---\n" RESET);
    if (saleCount == 0) {
        printf(YELLOW "No sales recorded yet.\n" RESET);
    }
    for (i = 0; i < saleCount; i++) {
        printf("#%d | %-8s | Customer: %-20s | PC-%02d | %dh %02dm | PHP %.2f\n",
               i + 1,
               sales[i].customerId == WALKIN ? "Walk-in" : "Member",
               customerName(sales[i].customerId),
               sales[i].pcId, sales[i].minutes / 60, sales[i].minutes % 60,
               sales[i].amount);
    }
    printf(YELLOW "-------------------------------------\n" RESET);
    printf("Total Sales: PHP %.2f\n", totalSales);
}
void saveData() {
    FILE *f;
    int i;

    f = fopen("shop_customers.txt", "w");
    if (f != NULL) {
        for (i = 0; i < customerCount; i++) {
            fprintf(f, "%d\n%s\n%s\n", customers[i].id, customers[i].name, customers[i].contact);
        }
        fclose(f);
    } else {
        printf("ERROR: Could not save shop_customers.txt\n");
    }

    f = fopen("shop_computers.txt", "w");
    if (f != NULL) {
        for (i = 0; i < MAX_COMPUTERS; i++) {
            fprintf(f, "%d %d %ld\n", computers[i].status, computers[i].customerId,
                    computers[i].startTime);
        }
        fclose(f);
    } else {
        printf("ERROR: Could not save shop_computers.txt\n");
    }

    f = fopen("shop_sales.txt", "w");
    if (f != NULL) {
        for (i = 0; i < saleCount; i++) {
            fprintf(f, "%d %d %d %.2f\n", sales[i].customerId, sales[i].pcId,
                    sales[i].minutes, sales[i].amount);
        }
        fclose(f);
    } else {
        printf("ERROR: Could not save shop_sales.txt\n");
    }
}

int readFileLine(FILE *f, char text[], int size) {
    if (fgets(text, size, f) == NULL) {
        return 0;
    }
    text[strcspn(text, "\r\n")] = '\0';
    return 1;
}

void loadData() {
    FILE *f;
    int i, status, customerId;
    long startTime;
    char idLine[100], name[100], contact[100];
    struct Sale s;

    for (i = 0; i < MAX_COMPUTERS; i++) {
        computers[i].id = i + 1;
        sprintf(computers[i].name, "PC-%02d", i + 1);
        computers[i].status = AVAILABLE;
        computers[i].customerId = 0;
        computers[i].startTime = 0;
    }

    f = fopen("shop_customers.txt", "r");
    if (f != NULL) {
        while (customerCount < MAX_CUSTOMERS &&
               readFileLine(f, idLine, 100) &&
               readFileLine(f, name, 100) &&
               readFileLine(f, contact, 100)) {
            if (atoi(idLine) > 0 && isValidName(name) && strlen(name) < 50 &&
                isValidContact(contact) && findCustomer(atoi(idLine)) == -1 &&
                findByContact(contact) == -1) {
                customers[customerCount].id = atoi(idLine);
                strcpy(customers[customerCount].name, name);
                strcpy(customers[customerCount].contact, contact);
                customerCount++;
            }
        }
        fclose(f);
    }

    f = fopen("shop_computers.txt", "r");
    if (f != NULL) {
        for (i = 0; i < MAX_COMPUTERS; i++) {
            if (fscanf(f, "%d %d %ld", &status, &customerId, &startTime) != 3) {
                break;
            }
            if (status < AVAILABLE || status > IN_USE) {
                continue;
            }
            if (status != AVAILABLE && customerId != WALKIN && findCustomer(customerId) == -1) {
                continue;
            }
            if (status == IN_USE && startTime <= 0) {
                status = ASSIGNED;
            }
            if (status == AVAILABLE) {
                customerId = 0;
                startTime = 0;
            }
            computers[i].status = status;
            computers[i].customerId = customerId;
            computers[i].startTime = startTime;
        }
        fclose(f);
    }

0    f = fopen("shop_sales.txt", "r");
    if (f != NULL) {
        while (saleCount < MAX_SALES &&
               fscanf(f, "%d %d %d %lf", &s.customerId, &s.pcId, &s.minutes, &s.amount) == 4) {
            if (s.customerId != 0 && s.pcId >= 1 && s.pcId <= MAX_COMPUTERS && s.minutes >= 1) {
                sales[saleCount] = s;
                saleCount++;
                totalSales = totalSales + s.amount;
            }
        }
        fclose(f);
    }
}
