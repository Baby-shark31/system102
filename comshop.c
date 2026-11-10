/* =====================================================================
 *  COMPUTER SHOP MANAGEMENT SYSTEM
 *  A console-based C program for managing a small computer shop.
 *
 *  WHAT IT DOES
 *   1. Customer Management  - register, view, search, sort, edit, delete
 *   2. Computer Management  - check availability, assign a computer,
 *                             end a session and compute the bill
 *   3. Inventory            - track items (snacks, drinks, print pages...),
 *                             restock, search, sort, and sell items
 *   4. Sales                - view all sales and the daily sales report
 *   5. File handling        - everything is saved to / loaded from local
 *                             files, so nothing has to be typed in again
 *
 *  NOTES
 *   - Session time is typed in by the staff (hours + minutes).
 *   - Billing: rate per hour, charged per minute, with a minimum charge
 *     of MIN_BILLABLE_MIN minutes.
 *   - Records are saved automatically after every change.
 *
 *  COMPILE:  gcc computer_shop.c -o computer_shop
 *  RUN:      ./computer_shop        (Windows: computer_shop.exe)
 * ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ------------------------------ SETTINGS ------------------------------ */
#define MAX_CUSTOMERS   200
#define MAX_COMPUTERS    30
#define MAX_ITEMS       100
#define MAX_SALES      2000

#define NAME_LEN         50
#define CONTACT_LEN      20
#define PCNAME_LEN       20
#define DESC_LEN         70
#define DATE_LEN         11      /* "YYYY-MM-DD" + the '\0' at the end */

#define DEFAULT_PCS          10      /* computers created on the first run */
#define DEFAULT_RATE_PER_HR  20.00   /* PHP per hour                       */
#define MIN_BILLABLE_MIN     15      /* minimum minutes that get charged   */
#define LOW_STOCK_LEVEL       5      /* items at or below this are flagged */

#define CUSTOMER_FILE   "customers.dat"
#define COMPUTER_FILE   "computers.dat"
#define INVENTORY_FILE  "inventory.dat"
#define SALES_FILE      "sales.dat"

#define RESET   "\x1b[0m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"

/* Color roles - use these everywhere so the whole program looks the same */
#define COLOR_BORDER  BLUE      /* lines, separators, frames       */
#define COLOR_TITLE   YELLOW    /* screen / menu / table titles    */
#define COLOR_MENU    GREEN     /* menu options                    */
#define COLOR_OK      GREEN     /* success messages                */
#define COLOR_ERROR   RED       /* errors and warnings             */



/* Computer status codes */
enum { PC_AVAILABLE = 0, PC_IN_USE = 1, PC_MAINTENANCE = 2 };

/* Sale types */
enum { SALE_COMPUTER = 0, SALE_ITEM = 1 };

/* ------------------------------ RECORDS ------------------------------- */
typedef struct {
    int  id;
    char name[NAME_LEN];
    char contact[CONTACT_LEN];
} Customer;

typedef struct {
    int    id;
    char   name[PCNAME_LEN];   /* e.g. "PC-01"                         */
    int    status;             /* PC_AVAILABLE / PC_IN_USE / ...       */
    int    customerId;         /* who is using it (0 if nobody)        */
    double rate;               /* price per hour                       */
} Computer;

typedef struct {
    int    id;
    char   name[NAME_LEN];
    int    quantity;           /* stock on hand                        */
    double price;              /* price per piece                      */
} Item;

typedef struct {
    int    id;
    int    type;               /* SALE_COMPUTER or SALE_ITEM           */
    char   customerName[NAME_LEN];
    char   description[DESC_LEN];
    double amount;
    char   date[DATE_LEN];     /* YYYY-MM-DD                           */
} Sale;

/* ------------------- GLOBAL DATA (the "database") --------------------- */
Customer customers[MAX_CUSTOMERS];  int customerCount = 0;
Computer computers[MAX_COMPUTERS];  int computerCount = 0;
Item     items[MAX_ITEMS];          int itemCount     = 0;
Sale     sales[MAX_SALES];          int saleCount     = 0;

/* Function needed early by the input helpers */
void saveAll(void);

/* =====================================================================
 *  COLOR HELPERS
 * ===================================================================== */

/* Turn on ANSI color support (needed on Windows consoles) */
static void enableColors(void) {
#ifdef _WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD  mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004);   /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
#endif
}

/* Same look for every menu / screen title */
static void printTitle(const char *title) {
    printf("\n" COLOR_BORDER "========== " COLOR_TITLE "%s" COLOR_BORDER " ==========" RESET "\n", title);
}

/* =====================================================================
 *  INPUT HELPERS
 *  scanf() is easy to break (letters typed instead of numbers, leftover
 *  newlines...). These helpers read a whole line with fgets() and check
 *  it, so the program does not crash or loop forever on bad input.
 * ===================================================================== */

/* Remove spaces at the start and end of a string */
static void trim(char *s) {
    char  *start = s;
    size_t len;

    while (*start != '\0' && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

/* Read one line from the keyboard into buf (without the newline) */
static void readLine(char *buf, int size) {
    size_t len;

    if (fgets(buf, size, stdin) == NULL) {      /* keyboard input closed */
        printf("\nInput ended. Saving and exiting...\n");
        saveAll();
        exit(0);
    }

    len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {                                    /* line was too long: discard the rest */
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }
    trim(buf);
}

/* Ask for a whole number between min and max */
static int readIntRange(const char *prompt, int min, int max) {
    char  buf[64];
    char *end;
    long  value;

    for (;;) {
        printf("%s", prompt);
        readLine(buf, sizeof(buf));
        value = strtol(buf, &end, 10);
        if (end != buf && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf(COLOR_ERROR "  Invalid input. Enter a whole number from %d to %d.\n" RESET, min, max);
    }
}

/* Ask for a decimal number between min and max */
static double readMoney(const char *prompt, double min, double max) {
    char   buf[64];
    char  *end;
    double value;

    for (;;) {
        printf("%s", prompt);
        readLine(buf, sizeof(buf));
        value = strtod(buf, &end);
        if (end != buf && *end == '\0' && value >= min && value <= max)
            return value;
        printf(COLOR_ERROR "  Invalid amount. Enter a number from %.2f to %.2f.\n" RESET, min, max);
    }
}

/* Ask for text that must not be empty */
static void readRequiredText(const char *prompt, char *buf, int size) {
    for (;;) {
        printf("%s", prompt);
        readLine(buf, size);
        if (buf[0] != '\0') return;
        printf(COLOR_ERROR "  This field cannot be empty.\n" RESET);
    }
}

/* Ask a yes/no question. Returns 1 for yes, 0 for no */
static int confirm(const char *prompt) {
    char buf[16];
    printf("%s (y/n): ", prompt);
    readLine(buf, sizeof(buf));
    return (buf[0] == 'y' || buf[0] == 'Y');
}

static void pauseScreen(void) {
    char buf[8];
    printf("\nPress Enter to continue...");
    readLine(buf, sizeof(buf));
}

/* =====================================================================
 *  STRING, DATE AND MONEY HELPERS
 * ===================================================================== */

/* Compare two strings ignoring upper/lower case (like strcmp) */
static int compareIgnoreCase(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++;
        b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

/* Does text contain key (ignoring case)? Returns 1 or 0 */
static int containsIgnoreCase(const char *text, const char *key) {
    size_t tl = strlen(text), kl = strlen(key), i, j;

    if (kl == 0) return 1;
    for (i = 0; i + kl <= tl; i++) {
        for (j = 0; j < kl; j++) {
            if (tolower((unsigned char)text[i + j]) != tolower((unsigned char)key[j]))
                break;
        }
        if (j == kl) return 1;
    }
    return 0;
}

/* Put today's date in out as YYYY-MM-DD (out must hold DATE_LEN chars) */
static void getToday(char *out) {
    time_t     now = time(NULL);
    struct tm *t   = localtime(&now);
    strftime(out, DATE_LEN, "%Y-%m-%d", t);
}

/* Check that a string looks like YYYY-MM-DD */
static int isValidDate(const char *s) {
    int i, month, day;

    if (strlen(s) != 10) return 0;
    for (i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (s[i] != '-') return 0;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
    }
    month = atoi(s + 5);
    day   = atoi(s + 8);
    return (month >= 1 && month <= 12 && day >= 1 && day <= 31);
}

/* Round a money value to 2 decimal places */
static double roundMoney(double amount) {
    return (double)((long)(amount * 100.0 + 0.5)) / 100.0;
}

/* Minutes that will actually be charged (minimum applies) */
static int billableMinutes(int minutesUsed) {
    return (minutesUsed < MIN_BILLABLE_MIN) ? MIN_BILLABLE_MIN : minutesUsed;
}

/* Bill = (billable minutes / 60) x rate per hour */
static double computeBill(int minutesUsed, double ratePerHour) {
    return roundMoney((billableMinutes(minutesUsed) / 60.0) * ratePerHour);
}

/* =====================================================================
 *  LOOKUP HELPERS  (each returns an array index, or -1 if not found)
 * ===================================================================== */
static int findCustomer(int id) {
    int i;
    for (i = 0; i < customerCount; i++)
        if (customers[i].id == id) return i;
    return -1;
}

static int findComputer(int id) {
    int i;
    for (i = 0; i < computerCount; i++)
        if (computers[i].id == id) return i;
    return -1;
}

static int findItem(int id) {
    int i;
    for (i = 0; i < itemCount; i++)
        if (items[i].id == id) return i;
    return -1;
}

/* Which computer (index) is this customer using right now? -1 if none */
static int computerUsedBy(int customerId) {
    int i;
    for (i = 0; i < computerCount; i++)
        if (computers[i].status == PC_IN_USE && computers[i].customerId == customerId)
            return i;
    return -1;
}

/* Next free ID = highest existing ID + 1 */
static int nextCustomerId(void) {
    int i, max = 0;
    for (i = 0; i < customerCount; i++)
        if (customers[i].id > max) max = customers[i].id;
    return max + 1;
}

static int nextComputerId(void) {
    int i, max = 0;
    for (i = 0; i < computerCount; i++)
        if (computers[i].id > max) max = computers[i].id;
    return max + 1;
}

static int nextItemId(void) {
    int i, max = 0;
    for (i = 0; i < itemCount; i++)
        if (items[i].id > max) max = items[i].id;
    return max + 1;
}

static int nextSaleId(void) {
    int i, max = 0;
    for (i = 0; i < saleCount; i++)
        if (sales[i].id > max) max = sales[i].id;
    return max + 1;
}

/* =====================================================================
 *  FILE HANDLING
 *  Each file starts with the number of records, followed by the records
 *  themselves, written in one go with fwrite() and read with fread().
 * ===================================================================== */

/* Save 'count' records of 'size' bytes each. Returns 1 on success */
static int saveArray(const char *filename, const void *data, size_t size, int count) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) return 0;

    if (fwrite(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (count > 0 && fwrite(data, size, (size_t)count, fp) != (size_t)count) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

/* Load records from a file. Returns the record count, or -1 if the file
 * is missing or damaged (for example on the very first run). */
static int loadArray(const char *filename, void *data, size_t size, int maxCount) {
    int   count = 0;
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) return -1;

    if (fread(&count, sizeof(int), 1, fp) != 1 || count < 0 || count > maxCount) {
        fclose(fp);
        return -1;
    }
    if (count > 0 && fread(data, size, (size_t)count, fp) != (size_t)count) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return count;
}

/* Create the starting list of computers (first run only) */
static void setDefaultComputers(void) {
    int i;
    computerCount = DEFAULT_PCS;
    for (i = 0; i < DEFAULT_PCS; i++) {
        computers[i].id         = i + 1;
        snprintf(computers[i].name, PCNAME_LEN, "PC-%02d", i + 1);
        computers[i].status     = PC_AVAILABLE;
        computers[i].customerId = 0;
        computers[i].rate       = DEFAULT_RATE_PER_HR;
    }
}

void saveAll(void) {
    int ok = 1;
    ok &= saveArray(CUSTOMER_FILE,  customers, sizeof(Customer), customerCount);
    ok &= saveArray(COMPUTER_FILE,  computers, sizeof(Computer), computerCount);
    ok &= saveArray(INVENTORY_FILE, items,     sizeof(Item),     itemCount);
    ok &= saveArray(SALES_FILE,     sales,     sizeof(Sale),     saleCount);
    if (!ok) printf(COLOR_ERROR "  WARNING: some records could not be saved to file!\n" RESET);
}

static void loadAll(void) {
    int n;

    n = loadArray(CUSTOMER_FILE, customers, sizeof(Customer), MAX_CUSTOMERS);
    customerCount = (n < 0) ? 0 : n;

    n = loadArray(COMPUTER_FILE, computers, sizeof(Computer), MAX_COMPUTERS);
    if (n <= 0) setDefaultComputers();      /* no saved computers yet */
    else        computerCount = n;

    n = loadArray(INVENTORY_FILE, items, sizeof(Item), MAX_ITEMS);
    itemCount = (n < 0) ? 0 : n;

    n = loadArray(SALES_FILE, sales, sizeof(Sale), MAX_SALES);
    saleCount = (n < 0) ? 0 : n;
}

/* =====================================================================
 *  1. CUSTOMER MANAGEMENT
 * ===================================================================== */
static void printCustomerHeader(void) {
    printf(COLOR_TITLE "\n%-5s %-30s %-16s %s\n" RESET, "ID", "Name", "Contact", "Status");
    printf(COLOR_BORDER "----------------------------------------------------------------------" RESET "\n");
}

static void printCustomerRow(const Customer *c) {
    int pc = computerUsedBy(c->id);

    printf("%-5d %-30.30s %-16.16s ", c->id, c->name, c->contact);
    if (pc >= 0) printf("Using %s\n", computers[pc].name);
    else         printf("-\n");
}

static void registerCustomer(void) {
    Customer c;

    if (customerCount >= MAX_CUSTOMERS) {
        printf("  The customer list is full.\n");
        return;
    }
    printf("\n--- Register New Customer ---\n");
    readRequiredText("Full name      : ", c.name, NAME_LEN);
    readRequiredText("Contact number : ", c.contact, CONTACT_LEN);
    c.id = nextCustomerId();

    customers[customerCount++] = c;
    saveAll();
    printf("  Customer registered. Customer ID: %d\n", c.id);
}

static void viewCustomers(void) {
    int i;

    if (customerCount == 0) {
        printf("\n  No customers registered yet.\n");
        return;
    }
    printCustomerHeader();
    for (i = 0; i < customerCount; i++) printCustomerRow(&customers[i]);
    printf("\nTotal customers: %d\n", customerCount);
}

static void searchCustomers(void) {
    int  choice, i, found = 0;
    char key[NAME_LEN];

    printf("\n--- Search Customer ---\n");
    printf("1. Search by ID\n2. Search by name\n");
    choice = readIntRange("Choice: ", 1, 2);

    if (choice == 1) {
        int id = readIntRange("Enter customer ID: ", 1, 999999);
        i = findCustomer(id);
        if (i < 0) {
            printf(COLOR_ERROR "  Customer not found.\n" RESET);
        } else {
            printCustomerHeader();
            printCustomerRow(&customers[i]);
        }
    } else {
        readRequiredText("Enter name (or part of it): ", key, sizeof(key));
        for (i = 0; i < customerCount; i++) {
            if (containsIgnoreCase(customers[i].name, key)) {
                if (!found) printCustomerHeader();
                printCustomerRow(&customers[i]);
                found++;
            }
        }
        if (!found) printf("  No customer matches \"%s\".\n", key);
        else        printf("\n%d match(es) found.\n", found);
    }
}

/* Sorts a COPY of the list (bubble sort) so the saved order stays the same */
static void sortCustomers(void) {
    Customer temp[MAX_CUSTOMERS];
    int i, j, choice;

    if (customerCount == 0) {
        printf("\n  No customers registered yet.\n");
        return;
    }
    printf("\n--- Sort Customers ---\n");
    printf("1. By name (A-Z)\n2. By customer ID\n");
    choice = readIntRange("Choice: ", 1, 2);

    memcpy(temp, customers, sizeof(Customer) * (size_t)customerCount);

    for (i = 0; i < customerCount - 1; i++) {
        for (j = 0; j < customerCount - i - 1; j++) {
            int swap;
            if (choice == 1) swap = compareIgnoreCase(temp[j].name, temp[j + 1].name) > 0;
            else             swap = temp[j].id > temp[j + 1].id;

            if (swap) {
                Customer t = temp[j];
                temp[j]     = temp[j + 1];
                temp[j + 1] = t;
            }
        }
    }

    printCustomerHeader();
    for (i = 0; i < customerCount; i++) printCustomerRow(&temp[i]);
}

static void editCustomer(void) {
    int  idx, id;
    char buf[NAME_LEN];

    if (customerCount == 0) {
        printf("\n  No customers registered yet.\n");
        return;
    }
    id  = readIntRange("\nEnter customer ID to edit (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findCustomer(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Customer not found.\n" RESET);
        return;
    }

    printf("Editing: %s (%s)\n", customers[idx].name, customers[idx].contact);
    printf("Press Enter to keep the current value.\n");

    printf("New name    : ");
    readLine(buf, NAME_LEN);
    if (buf[0] != '\0') strcpy(customers[idx].name, buf);

    printf("New contact : ");
    readLine(buf, CONTACT_LEN);
    if (buf[0] != '\0') strcpy(customers[idx].contact, buf);

    saveAll();
    printf("  Customer record updated.\n");
}

static void deleteCustomer(void) {
    int idx, id, i;

    if (customerCount == 0) {
        printf("\n  No customers registered yet.\n");
        return;
    }
    id = readIntRange("\nEnter customer ID to delete (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findCustomer(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Customer not found.\n" RESET);
        return;
    }
    if (computerUsedBy(id) >= 0) {
        printf(COLOR_ERROR "  Cannot delete: this customer is currently using a computer.\n" RESET);
        return;
    }
    if (!confirm("Delete this customer record")) {
        printf("  Cancelled.\n");
        return;
    }
    for (i = idx; i < customerCount - 1; i++)   /* shift the rest up by one */
        customers[i] = customers[i + 1];
    customerCount--;

    saveAll();
    printf("  Customer deleted.\n");
}

static void customerMenu(void) {
    int choice;
    do {
        printTitle("CUSTOMER MANAGEMENT");
        printf(COLOR_MENU "1. Register new customer\n");
        printf("2. View all customers\n");
        printf("3. Search customer\n");
        printf("4. Sort customers\n");
        printf("5. Edit customer\n");
        printf("6. Delete customer\n");
        printf("0. Back to main menu\n" RESET);
        choice = readIntRange("Choose an option: ", 0, 6);

        switch (choice) {
            case 1: registerCustomer(); break;
            case 2: viewCustomers();    break;
            case 3: searchCustomers();  break;
            case 4: sortCustomers();    break;
            case 5: editCustomer();     break;
            case 6: deleteCustomer();   break;
        }
        if (choice != 0) pauseScreen();
    } while (choice != 0);
}

/* =====================================================================
 *  2. COMPUTER MANAGEMENT
 * ===================================================================== */
static const char *statusText(int status) {
    switch (status) {
        case PC_AVAILABLE: return "Available";
        case PC_IN_USE:    return "In use";
        default:           return "Maintenance";
    }
}

static void showComputers(void) {
    int i, available = 0;

    printf(COLOR_TITLE "\n%-5s %-10s %-14s %-13s %s\n" RESET, "ID", "Computer", "Rate (PHP/hr)", "Status", "Customer");
    printf(COLOR_BORDER "----------------------------------------------------------------------" RESET "\n");
    for (i = 0; i < computerCount; i++) {
        const Computer *pc = &computers[i];
        printf("%-5d %-10s %-14.2f %-13s ", pc->id, pc->name, pc->rate, statusText(pc->status));

        if (pc->status == PC_IN_USE) {
            int ci = findCustomer(pc->customerId);
            printf("%s\n", (ci >= 0) ? customers[ci].name : "(unknown)");
        } else {
            printf("-\n");
        }
        if (pc->status == PC_AVAILABLE) available++;
    }
    printf("\nAvailable: %d of %d computers\n", available, computerCount);
}

static void assignComputer(void) {
    int i, available = 0, custId, pcId, cIdx, pIdx;

    for (i = 0; i < computerCount; i++)
        if (computers[i].status == PC_AVAILABLE) available++;
    if (available == 0) {
        printf("\n  No computers are available right now.\n");
        return;
    }

    showComputers();
    custId = readIntRange("\nEnter customer ID (0 to cancel): ", 0, 999999);
    if (custId == 0) return;

    cIdx = findCustomer(custId);
    if (cIdx < 0) {
        printf(COLOR_ERROR "  Customer not found. Please register the customer first.\n" RESET);
        return;
    }
    if (computerUsedBy(custId) >= 0) {
        printf("  %s is already using a computer.\n", customers[cIdx].name);
        return;
    }

    pcId = readIntRange("Enter computer ID to assign (0 to cancel): ", 0, 999999);
    if (pcId == 0) return;

    pIdx = findComputer(pcId);
    if (pIdx < 0) {
        printf(COLOR_ERROR "  Computer not found.\n" RESET);
        return;
    }
    if (computers[pIdx].status != PC_AVAILABLE) {
        printf("  %s is not available (%s).\n", computers[pIdx].name, statusText(computers[pIdx].status));
        return;
    }

    computers[pIdx].status     = PC_IN_USE;
    computers[pIdx].customerId = custId;
    saveAll();
    printf("  %s assigned to %s. Session started.\n", computers[pIdx].name, customers[cIdx].name);
}

/* Print the bill, take payment, save the sale, and free the computer */
static void endSession(void) {
    int    i, inUse = 0, pcId, pIdx, cIdx;
    int    hours, mins, totalMin, billMin;
    double rate, bill, paid;
    Sale   s;

    if (saleCount >= MAX_SALES) {
        printf(COLOR_ERROR "\n  The sales record is full. Cannot record more sales.\n" RESET);
        return;
    }
    for (i = 0; i < computerCount; i++)
        if (computers[i].status == PC_IN_USE) inUse++;
    if (inUse == 0) {
        printf("\n  No computers are currently in use.\n");
        return;
    }

    showComputers();
    pcId = readIntRange("\nEnter computer ID to end the session (0 to cancel): ", 0, 999999);
    if (pcId == 0) return;

    pIdx = findComputer(pcId);
    if (pIdx < 0 || computers[pIdx].status != PC_IN_USE) {
        printf("  That computer is not in use.\n");
        return;
    }
    cIdx = findCustomer(computers[pIdx].customerId);
    rate = computers[pIdx].rate;

    printf("\nCustomer : %s\n", (cIdx >= 0) ? customers[cIdx].name : "(unknown)");
    printf("Computer : %s\n", computers[pIdx].name);

    do {
        hours = readIntRange("Hours used   (0-24): ", 0, 24);
        mins  = readIntRange("Minutes used (0-59): ", 0, 59);
        totalMin = hours * 60 + mins;
        if (totalMin == 0) printf("  Session time must be more than 0 minutes.\n");
    } while (totalMin == 0);

    billMin = billableMinutes(totalMin);
    bill    = computeBill(totalMin, rate);

    printf("\n" COLOR_BORDER "============== " COLOR_TITLE "RECEIPT" COLOR_BORDER " ==============" RESET "\n");
    printf("Customer  : %s\n", (cIdx >= 0) ? customers[cIdx].name : "(unknown)");
    printf("Computer  : %s\n", computers[pIdx].name);
    printf("Time used : %d hr %d min\n", hours, mins);
    if (billMin != totalMin)
        printf("Billed as : %d min (minimum charge)\n", billMin);
    printf("Rate      : PHP %.2f per hour\n", rate);
    printf("TOTAL     : PHP %.2f\n", bill);

    for (;;) {
        paid = readMoney("Amount paid: PHP ", 0.0, 1000000.0);
        if (paid + 0.001 >= bill) break;
        printf(COLOR_ERROR "  Payment is less than the total bill.\n" RESET);
    }
    printf("Change    : PHP %.2f\n", paid - bill);
    printf(COLOR_BORDER "=====================================" RESET "\n");

    /* Record the sale */
    s.id   = nextSaleId();
    s.type = SALE_COMPUTER;
    snprintf(s.customerName, NAME_LEN, "%s", (cIdx >= 0) ? customers[cIdx].name : "Unknown");
    snprintf(s.description, DESC_LEN, "%s, %d min @ %.2f/hr", computers[pIdx].name, billMin, rate);
    s.amount = bill;
    getToday(s.date);
    sales[saleCount++] = s;

    /* Free the computer */
    computers[pIdx].status     = PC_AVAILABLE;
    computers[pIdx].customerId = 0;

    saveAll();
    printf("  Session ended. %s is now available.\n", computers[pIdx].name);
}

static void setComputerRate(void) {
    int id, idx;

    showComputers();
    id = readIntRange("\nEnter computer ID (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findComputer(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Computer not found.\n" RESET);
        return;
    }
    computers[idx].rate = readMoney("New rate per hour (PHP): ", 1.0, 10000.0);
    saveAll();
    printf("  Rate updated for %s.\n", computers[idx].name);
}

/* Switch a computer between "Available" and "Maintenance" */
static void toggleMaintenance(void) {
    int id, idx;

    showComputers();
    id = readIntRange("\nEnter computer ID (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findComputer(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Computer not found.\n" RESET);
        return;
    }
    if (computers[idx].status == PC_IN_USE) {
        printf("  %s is in use. End the session first.\n", computers[idx].name);
        return;
    }
    if (computers[idx].status == PC_AVAILABLE) computers[idx].status = PC_MAINTENANCE;
    else                                       computers[idx].status = PC_AVAILABLE;

    saveAll();
    printf("  %s is now: %s\n", computers[idx].name, statusText(computers[idx].status));
}

static void addComputer(void) {
    Computer pc;

    if (computerCount >= MAX_COMPUTERS) {
        printf("  The computer list is full.\n");
        return;
    }
    pc.id = nextComputerId();
    snprintf(pc.name, PCNAME_LEN, "PC-%02d", pc.id);
    pc.status     = PC_AVAILABLE;
    pc.customerId = 0;
    pc.rate       = readMoney("Rate per hour (PHP): ", 1.0, 10000.0);

    computers[computerCount++] = pc;
    saveAll();
    printf(COLOR_OK "  %s added.\n" RESET, pc.name);
}

static void computerMenu(void) {
    int choice;
    do {
        printTitle("COMPUTER MANAGEMENT");
        printf(COLOR_MENU "1. Check computer availability\n");
        printf("2. Assign computer to customer\n");
        printf("3. End session and compute bill\n");
        printf("4. Change a computer's hourly rate\n");
        printf("5. Set / clear maintenance mode\n");
        printf("6. Add a new computer\n");
        printf("0. Back to main menu\n" RESET);
        choice = readIntRange("Choose an option: ", 0, 6);

        switch (choice) {
            case 1: showComputers();      break;
            case 2: assignComputer();     break;
            case 3: endSession();         break;
            case 4: setComputerRate();    break;
            case 5: toggleMaintenance();  break;
            case 6: addComputer();        break;
        }
        if (choice != 0) pauseScreen();
    } while (choice != 0);
}

/* =====================================================================
 *  3. INVENTORY MANAGEMENT
 * ===================================================================== */
static void printItemHeader(void) {
    printf(COLOR_TITLE "\n%-5s %-28s %-8s %-12s %s\n" RESET, "ID", "Item", "Stock", "Price (PHP)", "Note");
    printf(COLOR_BORDER "----------------------------------------------------------------------" RESET "\n");
}

static void printItemRow(const Item *it) {
    printf("%-5d %-28.28s %-8d %-12.2f %s\n", it->id, it->name, it->quantity, it->price,
           (it->quantity == 0) ? "OUT OF STOCK" : (it->quantity <= LOW_STOCK_LEVEL) ? "LOW STOCK" : "");
}

static void addItem(void) {
    Item it;
    int  i;

    if (itemCount >= MAX_ITEMS) {
        printf("  The inventory list is full.\n");
        return;
    }
    printf("\n--- Add Inventory Item ---\n");
    readRequiredText("Item name : ", it.name, NAME_LEN);

    for (i = 0; i < itemCount; i++) {
        if (compareIgnoreCase(items[i].name, it.name) == 0) {
            printf("  \"%s\" already exists (ID %d). Use 'Update stock / price' instead.\n",
                   items[i].name, items[i].id);
            return;
        }
    }
    it.quantity = readIntRange("Quantity  : ", 0, 100000);
    it.price    = readMoney("Price (PHP): ", 0.01, 100000.0);
    it.id       = nextItemId();

    items[itemCount++] = it;
    saveAll();
    printf("  Item added. Item ID: %d\n", it.id);
}

static void viewItems(void) {
    int i;

    if (itemCount == 0) {
        printf("\n  The inventory is empty.\n");
        return;
    }
    printItemHeader();
    for (i = 0; i < itemCount; i++) printItemRow(&items[i]);
    printf("\nTotal items: %d\n", itemCount);
}

static void updateItem(void) {
    int id, idx, change;

    if (itemCount == 0) {
        printf("\n  The inventory is empty.\n");
        return;
    }
    viewItems();
    id = readIntRange("\nEnter item ID to update (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findItem(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Item not found.\n" RESET);
        return;
    }

    change = readIntRange("Quantity to add (negative to remove, 0 for none): ", -100000, 100000);
    if (items[idx].quantity + change < 0) {
        printf("  Not enough stock to remove that many.\n");
        return;
    }
    items[idx].quantity += change;

    {
        double newPrice = readMoney("New price (0 to keep the current price): ", 0.0, 100000.0);
        if (newPrice > 0.0) items[idx].price = newPrice;
    }

    saveAll();
    printf("  %s updated. Stock: %d, Price: PHP %.2f\n", items[idx].name, items[idx].quantity, items[idx].price);
}

static void deleteItem(void) {
    int id, idx, i;

    if (itemCount == 0) {
        printf("\n  The inventory is empty.\n");
        return;
    }
    viewItems();
    id = readIntRange("\nEnter item ID to delete (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findItem(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Item not found.\n" RESET);
        return;
    }
    if (!confirm("Delete this item")) {
        printf("  Cancelled.\n");
        return;
    }
    for (i = idx; i < itemCount - 1; i++) items[i] = items[i + 1];
    itemCount--;

    saveAll();
    printf("  Item deleted.\n");
}

static void searchItems(void) {
    int  i, found = 0;
    char key[NAME_LEN];

    readRequiredText("\nEnter item name (or part of it): ", key, sizeof(key));
    for (i = 0; i < itemCount; i++) {
        if (containsIgnoreCase(items[i].name, key)) {
            if (!found) printItemHeader();
            printItemRow(&items[i]);
            found++;
        }
    }
    if (!found) printf("  No item matches \"%s\".\n", key);
}

static void sortItems(void) {
    Item temp[MAX_ITEMS];
    int  i, j, choice;

    if (itemCount == 0) {
        printf("\n  The inventory is empty.\n");
        return;
    }
    printf("\n--- Sort Inventory ---\n");
    printf("1. By name (A-Z)\n2. By stock (lowest first)\n");
    choice = readIntRange("Choice: ", 1, 2);

    memcpy(temp, items, sizeof(Item) * (size_t)itemCount);

    for (i = 0; i < itemCount - 1; i++) {
        for (j = 0; j < itemCount - i - 1; j++) {
            int swap;
            if (choice == 1) swap = compareIgnoreCase(temp[j].name, temp[j + 1].name) > 0;
            else             swap = temp[j].quantity > temp[j + 1].quantity;

            if (swap) {
                Item t = temp[j];
                temp[j]     = temp[j + 1];
                temp[j + 1] = t;
            }
        }
    }

    printItemHeader();
    for (i = 0; i < itemCount; i++) printItemRow(&temp[i]);
}

/* Sell an item: reduces stock and records the sale */
static void sellItem(void) {
    int    id, idx, qty, custId, cIdx = -1;
    double total, paid;
    char   prompt[80];
    Sale   s;

    if (saleCount >= MAX_SALES) {
        printf(COLOR_ERROR "\n  The sales record is full. Cannot record more sales.\n" RESET);
        return;
    }
    if (itemCount == 0) {
        printf("\n  The inventory is empty.\n");
        return;
    }
    viewItems();
    id = readIntRange("\nEnter item ID to sell (0 to cancel): ", 0, 999999);
    if (id == 0) return;
    idx = findItem(id);
    if (idx < 0) {
        printf(COLOR_ERROR "  Item not found.\n" RESET);
        return;
    }
    if (items[idx].quantity == 0) {
        printf("  %s is out of stock.\n", items[idx].name);
        return;
    }

    snprintf(prompt, sizeof(prompt), "Quantity (1-%d): ", items[idx].quantity);
    qty = readIntRange(prompt, 1, items[idx].quantity);

    custId = readIntRange("Customer ID (0 for walk-in customer): ", 0, 999999);
    if (custId != 0) {
        cIdx = findCustomer(custId);
        if (cIdx < 0) {
            printf(COLOR_ERROR "  Customer not found.\n" RESET);
            return;
        }
    }

    total = roundMoney(qty * items[idx].price);
    printf("\nTOTAL: PHP %.2f  (%d x %.2f)\n", total, qty, items[idx].price);

    for (;;) {
        paid = readMoney("Amount paid: PHP ", 0.0, 1000000.0);
        if (paid + 0.001 >= total) break;
        printf(COLOR_ERROR "  Payment is less than the total.\n" RESET);
    }
    printf("Change: PHP %.2f\n", paid - total);

    items[idx].quantity -= qty;

    s.id   = nextSaleId();
    s.type = SALE_ITEM;
    snprintf(s.customerName, NAME_LEN, "%s", (cIdx >= 0) ? customers[cIdx].name : "Walk-in");
    snprintf(s.description, DESC_LEN, "%d x %s", qty, items[idx].name);
    s.amount = total;
    getToday(s.date);
    sales[saleCount++] = s;

    saveAll();
    printf(COLOR_OK "  Sale recorded. Remaining stock of %s: %d\n" RESET, items[idx].name, items[idx].quantity);
}

static void inventoryMenu(void) {
    int choice;
    do {
        printTitle("INVENTORY MANAGEMENT");
        printf(COLOR_MENU "1. Add new item\n");
        printf("2. View all items\n");
        printf("3. Update stock / price\n");
        printf("4. Search item\n");
        printf("5. Sort items\n");
        printf("6. Sell item\n");
        printf("7. Delete item\n");
        printf("0. Back to main menu\n" RESET);
        choice = readIntRange("Choose an option: ", 0, 7);

        switch (choice) {
            case 1: addItem();     break;
            case 2: viewItems();   break;
            case 3: updateItem();  break;
            case 4: searchItems(); break;
            case 5: sortItems();   break;
            case 6: sellItem();    break;
            case 7: deleteItem();  break;
        }
        if (choice != 0) pauseScreen();
    } while (choice != 0);
}

/* =====================================================================
 *  4. SALES AND DAILY REPORT
 * ===================================================================== */
static const char *saleTypeText(int type) {
    return (type == SALE_COMPUTER) ? "Computer" : "Item";
}

static void viewSales(void) {
    int i;

    if (saleCount == 0) {
        printf("\n  No sales recorded yet.\n");
        return;
    }
    printf(COLOR_TITLE "\n%-5s %-11s %-9s %-18s %-32s %10s\n" RESET, "ID", "Date", "Type", "Customer", "Description", "Amount");
    printf(COLOR_BORDER "--------------------------------------------------------------------------------------" RESET "\n");
    for (i = 0; i < saleCount; i++) {
        printf("%-5d %-11s %-9s %-18.18s %-32.32s %10.2f\n",
               sales[i].id, sales[i].date, saleTypeText(sales[i].type),
               sales[i].customerName, sales[i].description, sales[i].amount);
    }
    printf("\nTotal records: %d\n", saleCount);
}

/* Prints the report to any output: the screen (stdout) or a text file */
static void printDailyReport(FILE *out, const char *date) {
    int    i, count = 0;
    double computerTotal = 0.0, itemTotal = 0.0;

    fprintf(out, "\n============== DAILY SALES REPORT: %s ==============\n", date);
    fprintf(out, "%-5s %-9s %-18s %-32s %10s\n", "ID", "Type", "Customer", "Description", "Amount");
    fprintf(out, "--------------------------------------------------------------------------\n");

    for (i = 0; i < saleCount; i++) {
        if (strcmp(sales[i].date, date) != 0) continue;

        fprintf(out, "%-5d %-9s %-18.18s %-32.32s %10.2f\n",
                sales[i].id, saleTypeText(sales[i].type),
                sales[i].customerName, sales[i].description, sales[i].amount);
        count++;
        if (sales[i].type == SALE_COMPUTER) computerTotal += sales[i].amount;
        else                                itemTotal     += sales[i].amount;
    }
    if (count == 0) fprintf(out, "No sales recorded for this date.\n");

    fprintf(out, "--------------------------------------------------------------------------\n");
    fprintf(out, "Transactions        : %d\n", count);
    fprintf(out, "Computer time sales : PHP %.2f\n", computerTotal);
    fprintf(out, "Item sales          : PHP %.2f\n", itemTotal);
    fprintf(out, "TOTAL SALES         : PHP %.2f\n", computerTotal + itemTotal);
}

static void dailySales(void) {
    char date[DATE_LEN], input[32], filename[64];

    getToday(date);
    printf("\nEnter date (YYYY-MM-DD) or press Enter for today [%s]: ", date);
    readLine(input, sizeof(input));
    if (input[0] != '\0') {
        if (!isValidDate(input)) {
            printf(COLOR_ERROR "  Invalid date. Use the format YYYY-MM-DD.\n" RESET);
            return;
        }
        strcpy(date, input);
    }

    printDailyReport(stdout, date);

    if (confirm("\nSave this report to a text file")) {
        FILE *fp;
        snprintf(filename, sizeof(filename), "sales_report_%s.txt", date);
        fp = fopen(filename, "w");
        if (fp == NULL) {
            printf(COLOR_ERROR "  Could not create %s\n" RESET, filename);
        } else {
            printDailyReport(fp, date);
            fclose(fp);
            printf(COLOR_OK "  Report saved to %s\n" RESET, filename);
        }
    }
}

static void salesMenu(void) {
    int choice;
    do {
        printTitle("SALES");
        printf(COLOR_MENU "1. View all sales records\n");
        printf("2. Daily sales report\n");
        printf("0. Back to main menu\n" RESET);
        choice = readIntRange("Choose an option: ", 0, 2);

        switch (choice) {
            case 1: viewSales();   break;
            case 2: dailySales();  break;
        }
        if (choice != 0) pauseScreen();
    } while (choice != 0);
}

/* =====================================================================
 *  MAIN
 * ===================================================================== */
int main(void) {
    int choice;

    enableColors();
    loadAll();

    printf(COLOR_BORDER "=========================================\n" RESET);
    printf(COLOR_TITLE "   COMPUTER SHOP MANAGEMENT SYSTEM\n" RESET);
    printf(COLOR_BORDER "=========================================\n" RESET);
    printf("Loaded: %d customer(s), %d computer(s), %d item(s), %d sale record(s)\n",
           customerCount, computerCount, itemCount, saleCount);

    do {
        printTitle("MAIN MENU");
        printf(COLOR_MENU "1. Customer Management\n");
        printf("2. Computer Management\n");
        printf("3. Inventory Management\n");
        printf("4. Sales and Daily Report\n");
        printf("0. Save and Exit\n" RESET);
        choice = readIntRange("Choose an option: ", 0, 4);

        switch (choice) {
            case 1: customerMenu();  break;
            case 2: computerMenu();  break;
            case 3: inventoryMenu(); break;
            case 4: salesMenu();     break;
        }
    } while (choice != 0);

    saveAll();
    printf("\nAll records saved. Goodbye!\n");
    return 0;
}

