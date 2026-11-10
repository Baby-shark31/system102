#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>    /* turn off echo while a password is typed */
#include <unistd.h>
#endif

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define BLUE    "\033[1;34m"
#define YELLOW  "\033[1;33m"
#define GREEN   "\033[1;32m"

#define MAX_CUSTOMERS 50
#define MAX_COMPUTERS 10
#define MAX_SALES     1000
#define LINE_SIZE     256

#define CUSTOMER_FILE "customers.txt"
#define COMPUTER_FILE "computers.txt"
#define SALES_FILE    "sales.txt"

#define NAME_MAX_LEN    49      /* fits Customer.name[50] */
#define CONTACT_DIGITS  11

#define PASS_MIN        4
#define PASS_MAX        20
#define PASS_BUF        64      /* raw input buffer; longer input is rejected */
#define PASS_TRIES      3

typedef struct {
    int id;
    char name[50];
    char contact[20];
    int hasPassword;            /* 0 = older account created before passwords existed */
    unsigned int salt;
    unsigned long long passHash;
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
    double amount;
} Sale;


Customer customers[MAX_CUSTOMERS];
int customerCount = 0;

Computer computers[MAX_COMPUTERS] = {
    {1, "PC-01", "Available",   0, 0},
    {2, "PC-02", "Available",   0, 0},
    {3, "PC-03", "Available",   0, 0},
    {4, "PC-04", "Maintenance", 0, 0},
    {5, "PC-05", "Available",   0, 0}
};
int computerCount = 5;

Sale sales[MAX_SALES];
int saleCount = 0;

double totalSales = 0.0;
const double HOURLY_RATE = 20.0;


void loadData(void);
void saveData(void);
void registerCustomer(void);
int  checkAvailableComputers(void);
void assignComputer(void);
void startSession(void);
void endSession(void);
void viewCustomerRecords(void);
void editCustomer(void);
void deleteCustomer(void);
void promptNickname(char *out, int size);
void promptContact(char *out, int selfIdx);
void viewSalesRecords(void);
void viewComputerInventory(void);
void cancelAssignment(void);
void exitSystem(void);

void enableAnsiColors(void);
void clearInputBuffer(void);
int  readLine(char *buf, int size);
int  readInt(int *out);
void trim(char *s);
int  isValidName(const char *s);
int  isValidContact(const char *s);
int  isValidStatus(const char *s);
int  isValidPassword(const char *s);
int  readPassword(char *buf, int size);
unsigned long long hashPassword(unsigned int salt, const char *pw);
void setPassword(Customer *c, const char *pw);
int  checkPassword(const Customer *c, const char *pw);
int  promptNewPassword(Customer *c);
int  authenticateCustomer(int idx);
int  findCustomer(int id);
int  findCustomerByContact(const char *contact);
int  findCustomerPc(int customerId);
int  nextCustomerId(void);
int  billedMinutes(time_t start);
int  listAvailableComputers(void);
const char *customerName(int id);


int main(void) {
    int choice;

    enableAnsiColors();
    srand((unsigned int)time(NULL));
    loadData();

    do {
        printf(YELLOW "\n=====================================\n" RESET);
        printf(BOLD YELLOW "   COMPUTER SHOP REGISTRATION SYSTEM \n" RESET);
        printf(YELLOW "=====================================\n" RESET);

        printf(GREEN "1. Register Customer\n" RESET);
        printf(GREEN "2. Check Available Computers\n" RESET);
        printf(GREEN "3. Assign Computer\n" RESET);
        printf(GREEN "4. Start Session\n" RESET);
        printf(GREEN "5. End Session / Calculate Payment\n" RESET);
        printf(GREEN "6. View Customer Records\n" RESET);
        printf(GREEN "7. View Sales Records\n" RESET);
        printf(GREEN "8. View Computer Inventory\n" RESET);
        printf(GREEN "9. Cancel Assignment\n" RESET);
        printf(GREEN "10. Edit Customer\n" RESET);
        printf(GREEN "11. Delete Customer\n" RESET);
        printf(GREEN "12. Exit\n" RESET);

        printf(YELLOW "-------------------------------------\n" RESET);
        printf("Enter your choice (1-12): ");
        if (!readInt(&choice)) {
            choice = 0;
        }

        switch (choice) {
            case 1:  registerCustomer();      break;
            case 2:  checkAvailableComputers(); break;
            case 3:  assignComputer();        break;
            case 4:  startSession();          break;
            case 5:  endSession();            break;
            case 6:  viewCustomerRecords();   break;
            case 7:  viewSalesRecords();      break;
            case 8:  viewComputerInventory(); break;
            case 9:  cancelAssignment();      break;
            case 10: editCustomer();          break;
            case 11: deleteCustomer();        break;
            case 12:
                printf(BLUE "[SYSTEM] Saving data to files...\n" RESET);
                saveData();
                exitSystem();
                break;
            default:
                printf(YELLOW "Invalid choice! Please select between 1 and 12.\n" RESET);
        }
    } while (choice != 12);

    return 0;
}


/* ---------- Console / input helpers ---------- */

void enableAnsiColors(void) {
#ifdef _WIN32
    /* Lets older Windows consoles display the color codes properly. */
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode)) {
        SetConsoleMode(h, mode | 0x0004); /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    }
#endif
}

void clearInputBuffer(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF);
}

/* Returns 1 on success, 0 if the input was too long (buffer is emptied). */
int readLine(char *buf, int size) {
    if (fgets(buf, size, stdin) == NULL) {
        saveData();
        printf("\n");
        exit(0);
    }
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
        len--;
        if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = '\0';
        return 1;
    }
    /* No newline: either too long, or EOF reached mid-line. */
    if (len < (size_t)(size - 1)) {
        return 1;                   /* last line without newline, fits fine */
    }
    clearInputBuffer();
    buf[0] = '\0';
    return 0;
}

int readInt(int *out) {
    char buf[32];
    char *end;

    if (!readLine(buf, sizeof(buf))) return 0;
    if (buf[0] == '\0') return 0;

    long v = strtol(buf, &end, 10);
    if (end == buf) return 0;                 /* no digits at all */
    while (*end == ' ' || *end == '\t') end++;
    if (*end != '\0') return 0;
    if (v < 0 || v > 1000000) return 0;

    *out = (int)v;
    return 1;
}

void trim(char *s) {
    size_t start = 0;
    size_t len = strlen(s);
    while (s[start] != '\0' && isspace((unsigned char)s[start])) start++;
    memmove(s, s + start, len - start + 1);
    len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
}


/* ---------- Validation ---------- */

/* Nickname rules: letters (including accented / UTF-8 such as n-tilde),
   digits, spaces and the symbols _ - . ' are allowed. It needs at least one
   letter or digit, so a nickname can't be just punctuation or blank.
   '|' (the file delimiter) and control characters are never allowed. */
int isValidName(const char *s) {
    int alnum = 0;
    for (int i = 0; s[i] != '\0'; i++) {
        unsigned char c = (unsigned char)s[i];
        if (isalnum(c) || c >= 128) {
            alnum++;
        } else if (c != ' ' && c != '_' && c != '-' && c != '\'' && c != '.') {
            return 0;
        }
    }
    return alnum >= 1;
}

int isValidContact(const char *s) {
    if (strlen(s) != CONTACT_DIGITS) return 0;
    for (int i = 0; s[i] != '\0'; i++) {
        if (!isdigit((unsigned char)s[i])) return 0;
    }
    return 1;
}

int isValidStatus(const char *s) {
    return strcmp(s, "Available") == 0 || strcmp(s, "Assigned") == 0 ||
           strcmp(s, "In Use") == 0   || strcmp(s, "Maintenance") == 0;
}

/* Password rules: PASS_MIN to PASS_MAX printable characters (spaces allowed). */
int isValidPassword(const char *s) {
    size_t len = strlen(s);
    if (len < PASS_MIN || len > PASS_MAX) return 0;
    for (size_t i = 0; i < len; i++) {
        if (!isprint((unsigned char)s[i])) return 0;
    }
    return 1;
}


/* ---------- Passwords ---------- */

/* Reads a line with the typed characters hidden when the program runs in a
   real terminal. If input is not a real console (some IDE run windows),
   it falls back to a normal visible line so it can never hang.
   Returns 1 on success, 0 if the input was too long. */
int readPassword(char *buf, int size) {
#ifdef _WIN32
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode = 0;
    int tty = (hIn != INVALID_HANDLE_VALUE && GetConsoleMode(hIn, &oldMode));
    if (tty) SetConsoleMode(hIn, oldMode & ~ENABLE_ECHO_INPUT);
#else
    struct termios oldt, newt;
    int tty = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldt) == 0;
    if (tty) {
        newt = oldt;
        newt.c_lflag &= ~(tcflag_t)ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    }
#endif

    int ok = 1;
    int gotLine = (fgets(buf, size, stdin) != NULL);

#ifdef _WIN32
    if (tty) SetConsoleMode(hIn, oldMode);
#else
    if (tty) tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    if (tty) printf("\n");     /* the Enter key was not echoed */

    if (!gotLine) {
        saveData();
        printf("\n");
        exit(0);
    }

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[--len] = '\0';
        if (len > 0 && buf[len - 1] == '\r') buf[len - 1] = '\0';
    } else if (len >= (size_t)(size - 1)) {
        clearInputBuffer();
        buf[0] = '\0';
        ok = 0;
    }
    return ok;
}

/* FNV-1a (64-bit) over the salt bytes followed by the password.
   Good enough to avoid storing passwords as plain text in a small
   shop program; not meant to resist a determined attacker. */
unsigned long long hashPassword(unsigned int salt, const char *pw) {
    unsigned long long h = 1469598103934665603ULL;
    for (int i = 0; i < 4; i++) {
        h ^= (unsigned char)((salt >> (8 * i)) & 0xFFu);
        h *= 1099511628211ULL;
    }
    for (size_t i = 0; pw[i] != '\0'; i++) {
        h ^= (unsigned char)pw[i];
        h *= 1099511628211ULL;
    }
    return h;
}

void setPassword(Customer *c, const char *pw) {
    c->salt = ((unsigned int)rand() << 16) ^ (unsigned int)rand() ^ (unsigned int)time(NULL);
    c->passHash = hashPassword(c->salt, pw);
    c->hasPassword = 1;
}

int checkPassword(const Customer *c, const char *pw) {
    return c->hasPassword && hashPassword(c->salt, pw) == c->passHash;
}

/* Asks for a new password twice. Fills in c on success. */
int promptNewPassword(Customer *c) {
    while (1) {
        char p1[PASS_BUF], p2[PASS_BUF];

        printf("Create Password (%d-%d characters): ", PASS_MIN, PASS_MAX);
        if (!readPassword(p1, sizeof(p1)) || !isValidPassword(p1)) {
            printf(YELLOW "ERROR: Password must be %d-%d printable characters!\n" RESET,
                   PASS_MIN, PASS_MAX);
            continue;
        }

        printf("Confirm Password: ");
        if (!readPassword(p2, sizeof(p2)) || strcmp(p1, p2) != 0) {
            printf(YELLOW "ERROR: Passwords do not match! Try again.\n" RESET);
            memset(p1, 0, sizeof(p1));
            memset(p2, 0, sizeof(p2));
            continue;
        }

        setPassword(c, p1);
        memset(p1, 0, sizeof(p1));
        memset(p2, 0, sizeof(p2));
        return 1;
    }
}

/* Logs a customer in. Returns 1 if allowed, 0 if not.
   Accounts made before passwords existed are asked to set one first. */
int authenticateCustomer(int idx) {
    Customer *c = &customers[idx];

    if (!c->hasPassword) {
        printf(YELLOW "This account has no password yet. Please set one now.\n" RESET);
        promptNewPassword(c);
        saveData();
        printf(GREEN "Password saved.\n" RESET);
        return 1;
    }

    for (int attempt = 1; attempt <= PASS_TRIES; attempt++) {
        char pw[PASS_BUF];
        printf("Enter Password for %s: ", c->name);
        int ok = readPassword(pw, sizeof(pw)) && checkPassword(c, pw);
        memset(pw, 0, sizeof(pw));
        if (ok) return 1;

        if (attempt < PASS_TRIES) {
            printf(YELLOW "ERROR: Wrong password! (%d attempt(s) left)\n" RESET,
                   PASS_TRIES - attempt);
        }
    }
    printf(YELLOW "ERROR: Too many wrong attempts. Returning to menu.\n" RESET);
    return 0;
}


/* ---------- Lookups ---------- */

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

/* Highest existing ID + 1, so IDs never collide. */
int nextCustomerId(void) {
    int maxId = 0;
    for (int i = 0; i < customerCount; i++) {
        if (customers[i].id > maxId) maxId = customers[i].id;
    }
    /* A deleted customer's sales must not be inherited by a new customer. */
    for (int i = 0; i < saleCount; i++) {
        if (sales[i].customerId > maxId) maxId = sales[i].customerId;
    }
    return maxId + 1;
}

const char *customerName(int id) {
    int idx = findCustomer(id);
    return (idx == -1) ? "(deleted customer)" : customers[idx].name;
}

/* Same formula used for the live display and for the final bill:
   every started minute counts, minimum 1 minute. */
int billedMinutes(time_t start) {
    double seconds = difftime(time(NULL), start);
    if (seconds < 0) seconds = 0;           /* clock moved backwards */
    int minutes = (int)((seconds + 59) / 60);
    if (minutes < 1) minutes = 1;
    return minutes;
}


/* ---------- File handling ---------- */

/* Reads one line (without the newline). Returns 1 = ok, 0 = EOF,
   -1 = line was too long (discarded). */
static int readFileLine(FILE *f, char *line, size_t size) {
    if (fgets(line, (int)size, f) == NULL) return 0;
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[--len] = '\0';
    } else if (len == size - 1) {
        int ch;
        while ((ch = fgetc(f)) != '\n' && ch != EOF);
        line[0] = '\0';
        return -1;
    }
    if (len > 0 && line[len - 1] == '\r') line[len - 1] = '\0';
    return 1;
}

/* Keeps a copy of a file that had unreadable lines so nothing is lost
   when the program saves a cleaned version. */
static void backupBadFile(const char *path, int badLines) {
    char bak[64];
    snprintf(bak, sizeof(bak), "%s.bak", path);
    remove(bak);
    if (rename(path, bak) == 0) {
        printf(YELLOW "WARNING: %d unreadable line(s) in %s. Original kept as %s.\n" RESET,
               badLines, path, bak);
    } else {
        printf(YELLOW "WARNING: %d unreadable line(s) in %s were skipped.\n" RESET,
               badLines, path);
    }
}

void loadData(void) {
    printf(BLUE "[SYSTEM] Loading data (customers, computers, sessions, sales, inventory)...\n" RESET);

    FILE *f;
    char line[LINE_SIZE];
    int r, bad;

    /* ----- customers ----- */
    f = fopen(CUSTOMER_FILE, "r");
    if (f) {
        bad = 0;
        while ((r = readFileLine(f, line, sizeof(line))) != 0) {
            if (r < 0) { bad++; continue; }
            if (line[0] == '\0') continue;
            if (customerCount >= MAX_CUSTOMERS) { bad++; continue; }

            Customer c;
            memset(&c, 0, sizeof(c));
            char pwField[32] = "";
            int fields = sscanf(line, "%d|%49[^|]|%19[^|\n]|%24[^\n]",
                                &c.id, c.name, c.contact, pwField);
            if (fields < 3) {
                bad++; continue;
            }
            trim(c.name);
            trim(c.contact);

            /* 3 fields = older account without a password; 4 = salt+hash in hex */
            if (fields == 4) {
                int hexOk = (strlen(pwField) == 24);
                for (int i = 0; hexOk && i < 24; i++) {
                    if (!isxdigit((unsigned char)pwField[i])) hexOk = 0;
                }
                if (!hexOk ||
                    sscanf(pwField, "%8x%16llx", &c.salt, &c.passHash) != 2) {
                    bad++; continue;
                }
                c.hasPassword = 1;
            }
            if (c.id <= 0 || !isValidName(c.name) || !isValidContact(c.contact) ||
                findCustomer(c.id) != -1 || findCustomerByContact(c.contact) != -1) {
                bad++; continue;
            }
            customers[customerCount++] = c;
        }
        fclose(f);
        if (bad > 0) backupBadFile(CUSTOMER_FILE, bad);
    }

    /* ----- computers ----- */
    f = fopen(COMPUTER_FILE, "r");
    if (f) {
        Computer tmp[MAX_COMPUTERS];
        int n = 0;
        bad = 0;
        while ((r = readFileLine(f, line, sizeof(line))) != 0) {
            if (r < 0) { bad++; continue; }
            if (line[0] == '\0') continue;
            if (n >= MAX_COMPUTERS) { bad++; continue; }

            Computer c;
            long long start = 0;
            memset(&c, 0, sizeof(c));
            if (sscanf(line, "%d|%29[^|]|%19[^|]|%d|%lld",
                       &c.id, c.name, c.status, &c.customerId, &start) != 5) {
                bad++; continue;
            }
            /* The program uses (id - 1) as the array index, so IDs must be 1..n */
            if (c.id != n + 1 || c.name[0] == '\0' || !isValidStatus(c.status) ||
                c.customerId < 0) {
                bad++; continue;
            }
            c.startTime = (time_t)start;

            int needsUser = (strcmp(c.status, "Assigned") == 0 ||
                             strcmp(c.status, "In Use") == 0);
            if (needsUser && findCustomer(c.customerId) == -1) {
                /* assigned to someone who no longer exists: free the PC */
                strcpy(c.status, "Available");
                c.customerId = 0;
                c.startTime = 0;
                bad++;
            } else if (!needsUser) {
                c.customerId = 0;
                c.startTime = 0;
            } else if (strcmp(c.status, "In Use") == 0 &&
                       (c.startTime <= 0 || c.startTime > time(NULL) + 86400)) {
                /* impossible start time: fall back to Assigned so nobody is overbilled */
                strcpy(c.status, "Assigned");
                c.startTime = 0;
                bad++;
            } else if (strcmp(c.status, "Assigned") == 0) {
                c.startTime = 0;
            }
            tmp[n++] = c;
        }
        fclose(f);
        if (n > 0) {
            memcpy(computers, tmp, sizeof(Computer) * (size_t)n);
            computerCount = n;
        }
        if (bad > 0) backupBadFile(COMPUTER_FILE, bad);
    }

    /* ----- sales ----- */
    f = fopen(SALES_FILE, "r");
    if (f) {
        bad = 0;
        while ((r = readFileLine(f, line, sizeof(line))) != 0) {
            if (r < 0) { bad++; continue; }
            if (line[0] == '\0') continue;
            if (saleCount >= MAX_SALES) { bad++; continue; }

            Sale s;
            if (sscanf(line, "%d|%d|%d|%lf",
                       &s.customerId, &s.pcId, &s.minutes, &s.amount) != 4) {
                bad++; continue;
            }
            if (s.customerId <= 0 || s.pcId < 1 || s.pcId > computerCount ||
                s.minutes < 1 || s.amount < 0) {
                bad++; continue;
            }
            sales[saleCount++] = s;
            totalSales += s.amount;
        }
        fclose(f);
        if (bad > 0) backupBadFile(SALES_FILE, bad);
    }

    printf(BLUE "[SYSTEM] Loaded %d customer(s), %d computer(s), %d sale(s).\n" RESET,
           customerCount, computerCount, saleCount);
}

void saveData(void) {
    FILE *f;

    f = fopen(CUSTOMER_FILE, "w");
    if (f) {
        for (int i = 0; i < customerCount; i++) {
            fprintf(f, "%d|%s|%s", customers[i].id, customers[i].name, customers[i].contact);
            if (customers[i].hasPassword) {
                fprintf(f, "|%08x%016llx", customers[i].salt, customers[i].passHash);
            }
            fprintf(f, "\n");
        }
        if (fclose(f) != 0) printf(YELLOW "ERROR: Problem writing %s!\n" RESET, CUSTOMER_FILE);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, CUSTOMER_FILE);
    }

    f = fopen(COMPUTER_FILE, "w");
    if (f) {
        for (int i = 0; i < computerCount; i++) {
            fprintf(f, "%d|%s|%s|%d|%lld\n", computers[i].id, computers[i].name, computers[i].status,
                    computers[i].customerId, (long long)computers[i].startTime);
        }
        if (fclose(f) != 0) printf(YELLOW "ERROR: Problem writing %s!\n" RESET, COMPUTER_FILE);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, COMPUTER_FILE);
    }

    f = fopen(SALES_FILE, "w");
    if (f) {
        for (int i = 0; i < saleCount; i++) {
            fprintf(f, "%d|%d|%d|%.2f\n", sales[i].customerId, sales[i].pcId,
                    sales[i].minutes, sales[i].amount);
        }
        if (fclose(f) != 0) printf(YELLOW "ERROR: Problem writing %s!\n" RESET, SALES_FILE);
    } else {
        printf(YELLOW "ERROR: Could not save %s!\n" RESET, SALES_FILE);
    }
}


/* ---------- Menu actions ---------- */

/* Asks until a valid nickname is typed. out must hold 50 chars. */
void promptNickname(char *out, int size) {
    while (1) {
        printf("Enter Nickname: ");
        if (!readLine(out, size)) {
            printf(YELLOW "ERROR: Nickname is too long (max %d characters)!\n" RESET, NAME_MAX_LEN);
            continue;
        }
        trim(out);
        if (!isValidName(out)) {
            printf(YELLOW "ERROR: Nickname needs at least one letter or number and may only "
                          "contain letters, numbers, spaces, _ - ' and .\n" RESET);
            continue;
        }
        return;
    }
}

/* Asks until a valid, unused contact number is typed. out must hold 20 chars.
   selfIdx is the customer being edited (their own number is allowed), or -1. */
void promptContact(char *out, int selfIdx) {
    while (1) {
        char tmp[64];
        printf("Enter Contact Number (11 digits, e.g. 09171234567): ");
        if (!readLine(tmp, sizeof(tmp))) {
            printf(YELLOW "ERROR: Contact number is too long!\n" RESET);
            continue;
        }
        trim(tmp);

        if (!isValidContact(tmp)) {
            printf(YELLOW "ERROR: Contact number must be exactly %d digits (numbers only)!\n" RESET,
                   CONTACT_DIGITS);
            continue;
        }

        int existing = findCustomerByContact(tmp);
        if (existing != -1 && existing != selfIdx) {
            printf(YELLOW "ERROR: This number is already registered to %s (ID #%d)! "
                          "Please enter a different number.\n" RESET,
                   customers[existing].name, customers[existing].id);
            continue;
        }
        strcpy(out, tmp);
        return;
    }
}

void registerCustomer(void) {
    printf(BOLD YELLOW "\n--- 1. REGISTER CUSTOMER ---\n" RESET);

    if (customerCount >= MAX_CUSTOMERS) {
        printf(YELLOW "ERROR: Customer list is full (max %d customers)!\n" RESET, MAX_CUSTOMERS);
        return;
    }

    Customer c;
    memset(&c, 0, sizeof(c));
    c.id = nextCustomerId();

    promptNickname(c.name, sizeof(c.name));
    promptContact(c.contact, -1);

    promptNewPassword(&c);

    customers[customerCount] = c;
    customerCount++;
    saveData();

    printf(GREEN "SUCCESS: Customer registered with ID #%d!\n" RESET, c.id);
}

int listAvailableComputers(void) {
    int count = 0;
    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "Available") == 0) {
            printf("ID: %d | Name: %s | Status: " GREEN "%s\n" RESET,
                   computers[i].id, computers[i].name, computers[i].status);
            count++;
        }
    }
    if (count == 0) {
        printf(YELLOW "No available computers right now.\n" RESET);
    }
    return count;
}

int checkAvailableComputers(void) {
    printf(BOLD YELLOW "\n--- 2. AVAILABLE COMPUTERS ---\n" RESET);
    return listAvailableComputers();
}

void assignComputer(void) {
    printf(BOLD YELLOW "\n--- 3. ASSIGN COMPUTER ---\n" RESET);
    int pcId, custId;

    if (customerCount == 0) {
        printf(YELLOW "ERROR: No customers yet. Register a customer first (option 1).\n" RESET);
        return;
    }

    if (listAvailableComputers() == 0) {
        return;
    }

    printf("Enter Customer ID: ");
    if (!readInt(&custId) || findCustomer(custId) == -1) {
        printf(YELLOW "ERROR: Customer ID not found!\n" RESET);
        return;
    }

    if (!authenticateCustomer(findCustomer(custId))) {
        return;
    }

    int busyPc = findCustomerPc(custId);
    if (busyPc != -1) {
        printf(YELLOW "ERROR: %s is already assigned to %s!\n" RESET,
               customerName(custId), computers[busyPc].name);
        return;
    }

    printf("Enter Computer ID to assign: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount &&
        strcmp(computers[pcId - 1].status, "Available") == 0) {
        strcpy(computers[pcId - 1].status, "Assigned");
        computers[pcId - 1].customerId = custId;
        computers[pcId - 1].startTime = 0;
        saveData();
        printf(GREEN "SUCCESS: %s is now assigned to %s.\n" RESET,
               computers[pcId - 1].name, customerName(custId));
        printf(GREEN "Use option 4 to start the session.\n" RESET);
    } else {
        printf(YELLOW "ERROR: Invalid selection or computer is not available!\n" RESET);
    }
}

void startSession(void) {
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
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount &&
        strcmp(computers[pcId - 1].status, "Assigned") == 0) {
        Computer *pc = &computers[pcId - 1];
        strcpy(pc->status, "In Use");
        pc->startTime = time(NULL);
        saveData();

        char timeStr[32] = "";
        struct tm *tmInfo = localtime(&pc->startTime);
        if (tmInfo == NULL ||
            strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", tmInfo) == 0) {
            strcpy(timeStr, "(time unavailable)");
        }
        printf(GREEN "SUCCESS: Session started for %s on %s at %s.\n" RESET,
               customerName(pc->customerId), pc->name, timeStr);
    } else {
        printf(YELLOW "ERROR: Invalid selection or computer is not assigned!\n" RESET);
    }
}

void endSession(void) {
    printf(BOLD YELLOW "\n--- 5. END SESSION & CALCULATE PAYMENT ---\n" RESET);
    int pcId;
    int count = 0;

    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "In Use") == 0) {
            printf("ID: %d | Name: %s | Customer: " YELLOW "%s" RESET " | Time so far: %d min\n",
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

    printf("Enter Computer ID ending session: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount &&
        strcmp(computers[pcId - 1].status, "In Use") == 0) {

        if (saleCount >= MAX_SALES) {
            printf(YELLOW "ERROR: Sales records are full (max %d)! Cannot end session.\n" RESET,
                   MAX_SALES);
            return;
        }

        Computer *pc = &computers[pcId - 1];

        int minutes = billedMinutes(pc->startTime);
        double payment = (minutes / 60.0) * HOURLY_RATE;
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

void viewCustomerRecords(void) {
    printf(BOLD YELLOW "\n--- 6. CUSTOMER RECORDS ---\n" RESET);
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return;
    }
    for (int i = 0; i < customerCount; i++) {
        int pc = findCustomerPc(customers[i].id);
        printf("ID: %d | Nickname: %-20s | Contact: %s | PC: %s\n",
               customers[i].id, customers[i].name, customers[i].contact,
               (pc == -1) ? "-" : computers[pc].name);
    }
}

/* Shows ID + nickname, asks for an ID, and returns the array index
   (or -1 if the lookup failed). */
static int pickCustomer(void) {
    if (customerCount == 0) {
        printf(YELLOW "No customer records found.\n" RESET);
        return -1;
    }
    for (int i = 0; i < customerCount; i++) {
        printf("ID: %d | Nickname: %s\n", customers[i].id, customers[i].name);
    }
    int id;
    printf("Enter Customer ID: ");
    if (!readInt(&id) || findCustomer(id) == -1) {
        printf(YELLOW "ERROR: Customer ID not found!\n" RESET);
        return -1;
    }
    return findCustomer(id);
}

void editCustomer(void) {
    printf(BOLD YELLOW "\n--- 10. EDIT CUSTOMER ---\n" RESET);

    int idx = pickCustomer();
    if (idx == -1) return;
    if (!authenticateCustomer(idx)) return;

    Customer *c = &customers[idx];
    int choice;
    do {
        printf(YELLOW "\nEditing ID #%d | Nickname: %s | Contact: %s\n" RESET,
               c->id, c->name, c->contact);
        printf("1. Change Nickname\n");
        printf("2. Change Contact Number\n");
        printf("3. Change Password\n");
        printf("0. Done\n");
        printf("Choice: ");
        if (!readInt(&choice)) choice = -1;

        if (choice == 1) {
            char tmp[50];
            promptNickname(tmp, sizeof(tmp));
            strcpy(c->name, tmp);
            saveData();
            printf(GREEN "SUCCESS: Nickname updated.\n" RESET);
        } else if (choice == 2) {
            char tmp[20];
            promptContact(tmp, idx);
            strcpy(c->contact, tmp);
            saveData();
            printf(GREEN "SUCCESS: Contact number updated.\n" RESET);
        } else if (choice == 3) {
            promptNewPassword(c);
            saveData();
            printf(GREEN "SUCCESS: Password updated.\n" RESET);
        } else if (choice != 0) {
            printf(YELLOW "Invalid choice! Enter 0-3.\n" RESET);
        }
    } while (choice != 0);
}

void deleteCustomer(void) {
    printf(BOLD YELLOW "\n--- 11. DELETE CUSTOMER ---\n" RESET);

    int idx = pickCustomer();
    if (idx == -1) return;
    if (!authenticateCustomer(idx)) return;

    int pc = findCustomerPc(customers[idx].id);
    if (pc != -1) {
        printf(YELLOW "ERROR: %s is still assigned to %s! End the session (option 5) "
                      "or cancel the assignment (option 9) first.\n" RESET,
               customers[idx].name, computers[pc].name);
        return;
    }

    char answer[32];
    printf(YELLOW "Delete %s (ID #%d)? This cannot be undone. Type YES to confirm: " RESET,
           customers[idx].name, customers[idx].id);
    if (!readLine(answer, sizeof(answer))) answer[0] = '\0';
    trim(answer);
    for (int i = 0; answer[i] != '\0'; i++) {
        answer[i] = (char)toupper((unsigned char)answer[i]);
    }
    if (strcmp(answer, "YES") != 0) {
        printf(YELLOW "Delete cancelled.\n" RESET);
        return;
    }

    char name[50];
    strcpy(name, customers[idx].name);
    memmove(&customers[idx], &customers[idx + 1],
            sizeof(Customer) * (size_t)(customerCount - idx - 1));
    customerCount--;
    memset(&customers[customerCount], 0, sizeof(Customer));
    saveData();

    printf(GREEN "SUCCESS: %s was deleted. Past sales records are kept.\n" RESET, name);
}

void viewSalesRecords(void) {
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

void viewComputerInventory(void) {
    printf(BOLD YELLOW "\n--- 8. COMPUTER INVENTORY ---\n" RESET);
    for (int i = 0; i < computerCount; i++) {
        const char *statusColor;
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

/* Frees a PC that was assigned but whose session has not started yet. */
void cancelAssignment(void) {
    printf(BOLD YELLOW "\n--- 9. CANCEL ASSIGNMENT ---\n" RESET);
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
        printf(YELLOW "No pending assignments to cancel. "
                      "(Running sessions must be ended with option 5.)\n" RESET);
        return;
    }

    printf("Enter Computer ID to cancel: ");
    if (readInt(&pcId) && pcId > 0 && pcId <= computerCount &&
        strcmp(computers[pcId - 1].status, "Assigned") == 0) {
        Computer *pc = &computers[pcId - 1];
        printf(GREEN "SUCCESS: Assignment of %s to %s cancelled.\n" RESET,
               pc->name, customerName(pc->customerId));
        strcpy(pc->status, "Available");
        pc->customerId = 0;
        pc->startTime = 0;
        saveData();
    } else {
        printf(YELLOW "ERROR: Invalid selection or computer is not in 'Assigned' status!\n" RESET);
    }
}

void exitSystem(void) {
    int active = 0;
    for (int i = 0; i < computerCount; i++) {
        if (strcmp(computers[i].status, "In Use") == 0) active++;
    }
    if (active > 0) {
        printf(YELLOW "\nNOTE: %d session(s) are still running. They are saved and "
                      "will continue (and keep being billed) when the program restarts.\n" RESET,
               active);
    }
    printf(BLUE "\n[SYSTEM] Closing system...\n" RESET);
    printf(BOLD GREEN "Goodbye!\n" RESET);
}
