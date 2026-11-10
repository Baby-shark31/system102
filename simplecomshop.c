#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define BLUE    "\033[1;34m"
#define YELLOW  "\033[1;33m"
#define GREEN   "\033[1;32m"

#define MAX_CUSTOMERS 50
#define MAX_COMPUTERS 10
#define MAX_SALES     1000

#define CUSTOMER_FILE "customers.txt"
#define COMPUTER_FILE "computers.txt"
#define SALES_FILE    "sales.txt"

typedef struct {
    int id;
    char name[50];
    char contact[20];
} Customer;

typedef struct {
    int id;
    char name[30];
    char status[20];     
    int customerId;      
    time_t startTime;    
} Computer;

typedef struct {
    int customerId;
    int pcId;
    int minutes;
    float amount;
} Sale;


Customer customers[MAX_CUSTOMERS];
int customerCount = 0;

Computer computers[MAX_COMPUTERS] = {
    {1, "PC-01", "Available", 0, 0},
    {2, "PC-02", "Available", 0, 0},
    {3, "PC-03", "Available", 0, 0},
    {4, "PC-04", "Maintenance", 0, 0},
    {5, "PC-05", "Available", 0, 0}
};
int computerCount = 5;

Sale sales[MAX_SALES];
int saleCount = 0;

float totalSales = 0.0;
const float HOURLY_RATE = 20.0; 

void loadData();
void saveData();
void registerCustomer();
int  checkAvailableComputers();
void assignComputer();
void startSession();
void endSession();
void viewCustomerRecords();
void viewSalesRecords();
void viewComputerInventory();
void exitSystem();


void clearInputBuffer();
void readLine(char *buf, int size);
int  readInt(int *out);
void trim(char *s);
int  isValidName(const char *s);
int  isValidContact(const char *s);
int  findCustomer(int id);
int  findCustomerByContact(const char *contact);
int  findCustomerPc(int customerId);
const char *customerName(int id);


int main() {
    int choice;

    loadData();

    
    do {
        
        printf(YELLOW "\n=====================================\n" RESET);
        printf(BOLD YELLOW "   COMPUTER SHOP REGISTRATION SYSTEM \n" RESET);
        printf(YELLOW "=====================================\n" RESET);

        
        printf(GREEN  "1. Register Customer\n" RESET);
        printf(GREEN "2. Check Available Computers\n" RESET);
        printf(GREEN  "3. Assign Computer\n" RESET);
        printf(GREEN "4. Start Session\n" RESET);
        printf(GREEN  "5. End Session / Calculate Payment\n" RESET);
        printf(GREEN "6. View Customer Records\n" RESET);
        printf(GREEN  "7. View Sales Records\n" RESET);
        printf(GREEN "8. View Computer Inventory\n" RESET);
        printf(GREEN  "9. Exit\n" RESET);

        printf(YELLOW "-------------------------------------\n" RESET);
        printf("Enter your choice (1-9): ");
        if (!readInt(&choice)) {
            choice = 0;   
        }

        
        switch (choice) {
            case 1: 
            registerCustomer();
            break;
            case 2: 
            checkAvailableComputers();
            break;
            case 3: 
            assignComputer(); 
            break;
            case 4: 
            startSession(); 
            break;
            case 5: 
            endSession(); 
            break;
            case 6: 
            viewCustomerRecords(); 
            break;
            case 7: 
            viewSalesRecords(); 
            break;
            case 8: 
            viewComputerInventory(); 
            break;
            case 9: 
                printf(BLUE "[SYSTEM] Saving data to files...\n" RESET);
                saveData();
                exitSystem(); 
                break;
            default:
                printf(YELLOW "Invalid choice! Please select between 1 and 9.\n" RESET);
        }
    } while (choice != 9);

    return 0;
}

void clearInputBuffer() {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

void readLine(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) {
        saveData();
        printf("\n");
        exit(0);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        clearInputBuffer();
        buf[0] = '\0';
    }
}

int readInt(int *out) {
    char buf[32];
    char *end;
    readLine(buf, sizeof(buf));
    if (buf[0] == '\0') return 0;

    long v = strtol(buf, &end, 10);
    while (*end == ' ') end++;
    if (*end != '\0') return 0;
    if (v < 0 || v > 1000000) return 0;

    *out = (int)v;
    return 1;
}

void trim(char *s) {
    size_t start = 0;
    size_t len = strlen(s);
    while (s[start] == ' ') start++;
    memmove(s, s + start, len - start + 1);
    len = strlen(s);
    while (len > 0 && s[len - 1] == ' ') {
        s[--len] = '\0';
    }
}

int isValidName(const char *s) {
    int hasLetter = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (isalpha((unsigned char)s[i])) {
            hasLetter = 1;
        } else if (s[i] != ' ') {
            return 0;
        }
    }
    return hasLetter;
}

int isValidContact(const char *s) {
    if (strlen(s) != 11) return 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    return 1;
}

int findCustomer(int id) {
    for (int i = 0; i < customerCount; i++) {
        if (customers[i].id == id) return i;
    }
    return -1;
}

int findCustomerByContact(const char *contact) {
    for (int i = 0; i < customerCount; i++) {
        if (strcmp(customers[i].contact, contact) == 0) return i;
    }
    return -1;
}

int findCustomerPc(int customerId) {
    for (int i = 0; i < computerCount; i++) {
        if (computers[i].customerId == customerId &&
            (strcmp(computers[i].status, "Assigned") == 0 ||
             strcmp(computers[i].status, "In Use") == 0)) {
            return i;
        }
    }
    return -1;
}

const char *customerName(int id) {
    int idx = findCustomer(id);
    return (idx == -1) ? "Unknown" : customers[idx].name;
}

void loadData() {
    printf(BLUE "[SYSTEM] Loading data (customers, computers, sessions, sales, inventory)...\n" RESET);

    FILE *f;

    f = fopen(CUSTOMER_FILE, "r");
    if (f) {
        Customer c;
        while (customerCount < MAX_CUSTOMERS &&
               fscanf(f, "%d|%49[^|]|%19[^\n]\n", &c.id, c.name, c.contact) == 3) {
            customers[customerCount++] = c;
        }
        fclose(f);
    }

    f = fopen(COMPUTER_FILE, "r");
    if (f) {
        Computer tmp[MAX_COMPUTERS];
        Computer c;
        long long start;
        int n = 0;
        while (n < MAX_COMPUTERS &&
               fscanf(f, "%d|%29[^|]|%19[^|]|%d|%lld\n", &c.id, c.name, c.status, &c.customerId, &start) == 5) {
            c.startTime = (time_t)start;
            tmp[n++] = c;
        }
        fclose(f);
        if (n > 0) {
            memcpy(computers, tmp, sizeof(Computer) * n);
            computerCount = n;
        }
    }

    f = fopen(SALES_FILE, "r");
    if (f) {
        Sale s;
        while (saleCount < MAX_SALES &&
               fscanf(f, "%d|%d|%d|%f\n", &s.customerId, &s.pcId, &s.minutes, &s.amount) == 4) {
            sales[saleCount++] = s;
            totalSales += s.amount;
        }
        fclose(f);
    }

    printf(BLUE "[SYSTEM] Loaded %d customer(s), %d computer(s), %d sale(s).\n" RESET,
           customerCount, computerCount, saleCount);
}

void saveData() {
    FILE *f;

    f = fopen(CUSTOMER_FILE, "w");
    if (f) {
        for (int i = 0; i < customerCount; i++) {
            fprintf(f, "%d|%s|%s\n", customers[i].id, customers[i].name, customers[i].contact);
        }
        fclose(f);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, CUSTOMER_FILE);
    }

    f = fopen(COMPUTER_FILE, "w");
    if (f) {
        for (int i = 0; i < computerCount; i++) {
            fprintf(f, "%d|%s|%s|%d|%lld\n", computers[i].id, computers[i].name, computers[i].status,
                    computers[i].customerId, (long long)computers[i].startTime);
        }
        fclose(f);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, COMPUTER_FILE);
    }

    f = fopen(SALES_FILE, "w");
    if (f) {
        for (int i = 0; i < saleCount; i++) {
            fprintf(f, "%d|%d|%d|%.2f\n", sales[i].customerId, sales[i].pcId, sales[i].minutes, sales[i].amount);
        }
        fclose(f);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, SALES_FILE);
    }
}

void registerCustomer() {
    printf(BOLD YELLOW "\n--- 1. REGISTER CUSTOMER ---\n" RESET);

    if (customerCount >= MAX_CUSTOMERS) {
        printf(YELLOW "ERROR: Customer list is full (max %d customers)!\n" RESET, MAX_CUSTOMERS);
        return;
    }

    Customer c;
    c.id = customerCount + 1;
    
    do {
        printf("Enter Customer Name: ");
        readLine(c.name, sizeof(c.name));
        trim(c.name);

        if (!isValidName(c.name)) {
            printf(YELLOW "ERROR: Name must contain letters only!\n" RESET);
        }
    } while (!isValidName(c.name));

    while (1) {
        printf("Enter Contact Number (11 digits, e.g. 09171234567): ");
        readLine(c.contact, sizeof(c.contact));

        if (!isValidContact(c.contact)) {
            printf(YELLOW "ERROR: Contact number must be exactly 11 digits (numbers only)!\n" RESET);
            continue;
        }

        int existing = findCustomerByContact(c.contact);
        if (existing != -1) {
            printf(YELLOW "ERROR: This number is already registered to %s (ID #%d)!\n" RESET,
                   customers[existing].name, customers[existing].id);
            return;
        }
        break;
    }

    customers[customerCount] = c;
    customerCount++;
    saveData();

    printf(GREEN "SUCCESS: Customer registered with ID #%d!\n" RESET, c.id);
}


int checkAvailableComputers() {
    printf(BOLD YELLOW "\n--- 2. AVAILABLE COMPUTERS ---\n" RESET);
    int count = 0;
    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "Available") == 0) {
            printf("ID: %d | Name: %s | Status: " GREEN "%s\n" RESET, computers[i].id, computers[i].name, computers[i].status);
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No available computers right now.\n" RESET);
    }
    return count;
}

void assignComputer() {
    printf(BOLD YELLOW "\n--- 3. ASSIGN COMPUTER ---\n" RESET);
    int pcId, custId;

    if (customerCount == 0) {
        printf(YELLOW "ERROR: No customers yet. Register a customer first (option 1).\n" RESET);
        return;
    }

    if (checkAvailableComputers() == 0) {
        return;
    }

    printf("Enter Customer ID: ");
    if (!readInt(&custId) || findCustomer(custId) == -1) {
        printf(YELLOW "ERROR: Customer ID not found!\n" RESET);
        return;
    }

    int busyPc = findCustomerPc(custId);
    if (busyPc != -1) {
        printf(YELLOW "ERROR: %s is already assigned to %s!\n" RESET, customerName(custId), computers[busyPc].name);
        return;
    }
    
    printf("Enter Computer ID to assign: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount && strcmp(computers[pcId - 1].status, "Available") == 0) {
        strcpy(computers[pcId - 1].status, "Assigned");
        computers[pcId - 1].customerId = custId;
        saveData();
        printf(GREEN "SUCCESS: %s is now assigned to %s.\n" RESET, computers[pcId - 1].name, customerName(custId));
        printf(GREEN "Use option 4 to start the session.\n" RESET);
    } else {
        printf(YELLOW "ERROR: Invalid selection or computer is not available!\n" RESET);
    }
}

void startSession() {
    printf(BOLD YELLOW "\n--- 4. START SESSION ---\n" RESET);
    int pcId;
    int count = 0;

    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "Assigned") == 0) {
            printf("ID: %d | Name: %s | Customer: " YELLOW "%s\n" RESET,
                   computers[i].id, computers[i].name, customerName(computers[i].customerId));
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No assigned computers waiting. Assign a computer first (option 3).\n" RESET);
        return;
    }

    printf("Enter Computer ID to start: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount && strcmp(computers[pcId - 1].status, "Assigned") == 0) {
        Computer *pc = &computers[pcId - 1];
        strcpy(pc->status, "In Use");
        pc->startTime = time(NULL);
        saveData();

        char timeStr[32];
        strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", localtime(&pc->startTime));
        printf(GREEN "SUCCESS: Session started for %s on %s at %s.\n" RESET,
               customerName(pc->customerId), pc->name, timeStr);
    } else {
        printf(YELLOW "ERROR: Invalid selection or computer is not assigned!\n" RESET);
    }
}

void endSession() {
    printf(BOLD YELLOW "\n--- 5. END SESSION & CALCULATE PAYMENT ---\n" RESET);
    int pcId;
    int count = 0;

    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "In Use") == 0) {
            int mins = (int)(difftime(time(NULL), computers[i].startTime) / 60);
            printf("ID: %d | Name: %s | Customer: " YELLOW "%s" RESET " | Time so far: %d min\n",
                   computers[i].id, computers[i].name, customerName(computers[i].customerId), mins);
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No computers are currently in use.\n" RESET);
        return;
    }

    printf("Enter Computer ID ending session: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount && strcmp(computers[pcId - 1].status, "In Use") == 0) {
        if (saleCount >= MAX_SALES) {
            printf(YELLOW "ERROR: Sales records are full!\n" RESET);
            return;
        }

        Computer *pc = &computers[pcId - 1];

        double seconds = difftime(time(NULL), pc->startTime);
        int minutes = (int)((seconds + 59) / 60);
        if (minutes < 1) minutes = 1;

        float hours = minutes / 60.0f;
        float payment = hours * HOURLY_RATE;
        totalSales += payment;

        sales[saleCount].customerId = pc->customerId;
        sales[saleCount].pcId = pc->id;
        sales[saleCount].minutes = minutes;
        sales[saleCount].amount = payment;
        saleCount++;

        printf(YELLOW "-------------------------------------\n" RESET);
        printf("Customer: %s\n", customerName(pc->customerId));
        printf("Computer: %s\n", pc->name);
        printf("Total Time: %dh %dm\n", minutes / 60, minutes % 60);
        printf("Rate: PHP %.2f / hour\n", HOURLY_RATE);
        printf(BOLD GREEN "TOTAL PAYMENT: PHP %.2f\n" RESET, payment);

        strcpy(pc->status, "Available");
        pc->customerId = 0;
        pc->startTime = 0;
        saveData();

        printf(YELLOW "Status updated: %s is now Available.\n" RESET, pc->name);
    } else {
        printf(YELLOW "ERROR: Computer ID is invalid or not currently 'In Use'.\n" RESET);
    }
}

void viewCustomerRecords() {
    printf(BOLD YELLOW "\n--- 6. CUSTOMER RECORDS ---\n" RESET);
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return;
    }
    for (int i = 0; i < customerCount; i++) {
        int pc = findCustomerPc(customers[i].id);
        printf("ID: %d | Name: %-20s | Contact: %s | PC: %s\n",
               customers[i].id, customers[i].name, customers[i].contact,
               (pc == -1) ? "-" : computers[pc].name);
    }
}

void viewSalesRecords() {
    printf(BOLD YELLOW "\n--- 7. SALES RECORDS ---\n" RESET);
    if (saleCount == 0) {
        printf(YELLOW "No sales recorded yet.\n" RESET);
    }
    for (int i = 0; i < saleCount; i++) {
        printf("#%d | Customer: %-20s | PC: PC-%02d | Time: %dh %02dm | PHP %.2f\n",
               i + 1, customerName(sales[i].customerId), sales[i].pcId,
               sales[i].minutes / 60, sales[i].minutes % 60, sales[i].amount);
    }
    printf(YELLOW "-------------------------------------\n" RESET);
    printf(BOLD GREEN "Total Sales Accumulated: PHP %.2f\n" RESET, totalSales);
}

void viewComputerInventory() {
    printf(BOLD YELLOW "\n--- 8. COMPUTER INVENTORY ---\n" RESET);
    for (int i = 0; i < computerCount; i++) {
        const char *statusColor = RESET;
        if (strcmp(computers[i].status, "Available") == 0) statusColor = GREEN;
        else if (strcmp(computers[i].status, "In Use") == 0 ||
                 strcmp(computers[i].status, "Assigned") == 0) statusColor = YELLOW;
        else statusColor = BLUE;

        printf("ID: %d | Name: %-10s | Status: %s%-12s" RESET,
               computers[i].id, computers[i].name, statusColor, computers[i].status);

        if (computers[i].customerId != 0) {
            printf(" | User: %s", customerName(computers[i].customerId));
        }
        printf("\n");
    }
}

void exitSystem() {
    printf(BLUE "\n[SYSTEM] Closing connections...\n" RESET);
    printf(BOLD GREEN "Goodbye!\n" RESET);
}