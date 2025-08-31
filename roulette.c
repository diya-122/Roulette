#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <math.h>

#define FILENAME "users.txt"
#define MAX_USERS 100
#define MAX_HISTORY 50
#define STARTING_BALANCE 1000
#define PASSWORD_LENGTH 20
#define INACTIVITY_TIMEOUT 300 // 5 minutes in seconds

/* ====== COLOR MACROS ====== */
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define MAGENTA "\x1B[35m"
#define CYAN "\x1B[36m"
#define WHITE "\x1B[37m" // Added White for general text
#define BOLD "\x1B[1m"   // Added Bold
#define RESET "\x1B[0m"

/* ====== STRUCT DEFINITIONS ====== */
typedef struct {
    char username[50];
    char password[PASSWORD_LENGTH];
    int balance;
    int games_played;
    int games_won;
    int highest_win;
    bool isAdmin;
} User;

typedef struct {
    char username[50];
    char game_type[30];
    int bet_amount;
    int result;
    int payout;
    char timestamp[20];
} GameHistory;

/* ====== GLOBAL VARIABLES ====== */
GameHistory gameHistory[MAX_HISTORY];
int historyCount = 0;

/* ====== FUNCTION PROTOTYPES ====== */
/* User Management */
void encryptPassword(char *password);
void decryptPassword(char *password);
bool verifyPassword(const char *input, const char *stored);
int loadUser(User *user);
void saveUser(User user);
void updateUser(User user);
void changePassword(User *user);

/* Game Functions */
void spinRoulette(int *result, char *color);
void printWheel(void);
void displayRules(void);
void playRoulette(User *user);
void addGameHistory(User user, const char* type, int bet, int res, int payout);
void showGameHistory(const char* username);
void displayStats(User user);

/* Utility Functions */
void clearInputBuffer(void);
void getInput(char *buf, int size);
void checkTimeout(time_t lastActivity);

/* Admin Functions */
void adminMenu(User *user);
bool isAdminPassword(const char *input);

/* ====== FUNCTION IMPLEMENTATIONS ====== */

void clearInputBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void getInput(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        buf[strcspn(buf, "\n")] = '\0';
    }
}

void encryptPassword(char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        password[i] = password[i] + 3;
    }
}

void decryptPassword(char *password) {
    for (int i = 0; password[i] != '\0'; i++) {
        password[i] = password[i] - 3;
    }
}

bool verifyPassword(const char *input, const char *stored) {
    char decrypted[PASSWORD_LENGTH];
    strcpy(decrypted, stored);
    decryptPassword(decrypted);
    return strcmp(input, decrypted) == 0;
}

bool isAdminPassword(const char *input) {
    return strcmp(input, "TEAM16") == 0;
}

void saveUser(User user) {
    FILE *file = fopen(FILENAME, "a+");
    if (!file) {
        perror(RED "Error opening user file for saving" RESET);
        return;
    }
    
    char encrypted[PASSWORD_LENGTH];
    strcpy(encrypted, user.password);

    fprintf(file, "%s %s %d %d %d %d %d\n", user.username, encrypted, 
            user.balance, user.games_played, user.games_won, 
            user.highest_win, user.isAdmin ? 1 : 0);
    fclose(file);
}

int loadUser(User *user) {
    FILE *file = fopen(FILENAME, "r");
    if (!file) return 0; // File doesn't exist or cannot be opened, no users loaded.
    
    User temp;
    char encrypted_password_from_file[PASSWORD_LENGTH];
    int found = 0;
    int adminFlag;
    
    fseek(file, 0, SEEK_SET);

    while (fscanf(file, "%49s %19s %d %d %d %d %d", temp.username, encrypted_password_from_file,
                      &temp.balance, &temp.games_played, &temp.games_won, 
                      &temp.highest_win, &adminFlag) != EOF) {
        temp.isAdmin = (adminFlag == 1);
        
        strcpy(temp.password, encrypted_password_from_file); 
        
        if (strcmp(temp.username, user->username) == 0) {
            if (user->password[0] != '\0') { // Only verify if a password was provided for login
                char decrypted_stored_password[PASSWORD_LENGTH];
                strcpy(decrypted_stored_password, temp.password);
                decryptPassword(decrypted_stored_password);

                if (strcmp(user->password, decrypted_stored_password) != 0) {
                    fclose(file);
                    return -1; // Incorrect password
                }
            }
            
            *user = temp; 
            found = 1;
            break;
        }
    }
    fclose(file);
    return found;
}


void updateUser(User user) {
    FILE *file = fopen(FILENAME, "r");
    if (!file) {
        perror(RED "Error opening user file for update (read)" RESET);
        return;
    }
    
    User users[MAX_USERS];
    int count = 0;
    char encrypted_password_from_file[PASSWORD_LENGTH];
    int adminFlag;
    
    while (fscanf(file, "%49s %19s %d %d %d %d %d", users[count].username, encrypted_password_from_file,
                      &users[count].balance, &users[count].games_played, 
                      &users[count].games_won, &users[count].highest_win,
                      &adminFlag) != EOF) {
        users[count].isAdmin = (adminFlag == 1);
        strcpy(users[count].password, encrypted_password_from_file);
        
        if (strcmp(users[count].username, user.username) == 0) {
            users[count] = user; 
        }
        count++;
    }
    fclose(file);

    file = fopen(FILENAME, "w");
    if (!file) {
        perror(RED "Error opening user file for update (write)" RESET);
        return;
    }
    
    for (int i = 0; i < count; i++) {
        fprintf(file, "%s %s %d %d %d %d %d\n", users[i].username, users[i].password,
                users[i].balance, users[i].games_played, users[i].games_won, 
                users[i].highest_win, users[i].isAdmin ? 1 : 0);
    }
    fclose(file);
}


void printWheel() {
    printf(BOLD "\n  ===== ROULETTE WHEEL =====\n" RESET);
    for (int i = 0; i <= 36; i++) {
        if (i == 0) {
            printf(GREEN "00 " RESET);
        } else {
            bool isRedNumber;
            if ((i >= 1 && i <= 10) || (i >= 19 && i <= 28)) {
                isRedNumber = (i % 2 != 0);
            } else if ((i >= 11 && i <= 18) || (i >= 29 && i <= 36)) {
                isRedNumber = (i % 2 == 0);
            } else {
                isRedNumber = false; 
            }

            if (isRedNumber) {
                printf(RED "%02d " RESET, i);
            } else {
                printf(BLUE "%02d " RESET, i);
            }
        }
        if (i > 0 && i % 12 == 0) printf("\n  ");
    }
    printf(BOLD "\n  ==========================\n" RESET);
}


void spinRoulette(int *result, char *color) {
    *result = rand() % 37; // 0-36
    
    bool isRed = false;
    const int redNumbers[] = {1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36};
    
    if (*result == 0) {
        strcpy(color, GREEN "Green" RESET);
    } else {
        for (int i = 0; i < sizeof(redNumbers)/sizeof(redNumbers[0]); i++) {
            if (*result == redNumbers[i]) {
                isRed = true;
                break;
            }
        }
        if (isRed) {
            strcpy(color, RED "Red" RESET);
        } else {
            strcpy(color, BLUE "Black" RESET);
        }
    }
}

void displayRules() {
    printf(BOLD CYAN "\n====== Roulette Rules =======" RESET
           WHITE "\n1. Bet types:\n"
           "   - Single number (" YELLOW "35:1" RESET ")\n"
           "   - Even/Odd (" YELLOW "1:1" RESET ")\n"
           "   - Red/Black (" YELLOW "1:1" RESET ")\n"
           "   - High/Low (1-18/19-36) (" YELLOW "1:1" RESET ")\n"
           "   - Dozens (1-12, 13-24, 25-36) (" YELLOW "2:1" RESET ")\n"
           "   - Columns (" YELLOW "2:1" RESET ") " MAGENTA "[NEW]" RESET "\n"
           "2. Min bet: $10, Max bet: $1000\n\n" RESET);
}

void addGameHistory(User user, const char* type, int bet, int res, int payout) {
    if (historyCount >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            gameHistory[i] = gameHistory[i+1];
        }
        historyCount = MAX_HISTORY - 1;
    }
    
    time_t now = time(NULL);
    strftime(gameHistory[historyCount].timestamp, sizeof(gameHistory[historyCount].timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    strcpy(gameHistory[historyCount].username, user.username);
    strcpy(gameHistory[historyCount].game_type, type);
    gameHistory[historyCount].bet_amount = bet;
    gameHistory[historyCount].result = res;
    gameHistory[historyCount].payout = payout;
    historyCount++;
}

void playRoulette(User *user) {
    if (user->balance < 10) {
        printf(RED "Minimum bet is $10. Balance too low. Please top up or try again later.\n" RESET);
        return;
    }

    int betType, betNum = 0, betAmt;
    char gameType[30] = "Unknown";
    const int redNumbers[] = {1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36};

    printf(WHITE "\nYour Current Balance: $" GREEN "%d" RESET "\n", user->balance);
    
    printf(BOLD YELLOW "\n--- Place Your Bet ---\n" RESET);
    printf(WHITE "1) Single Number\n2) Even/Odd\n3) Red/Black\n"
           "4) High/Low (1-18/19-36)\n5) Dozens (1-12, 13-24, 25-36)\n6) Columns\n"
           BOLD CYAN "Enter your bet type choice: " RESET);
    if (scanf("%d", &betType) != 1 || betType < 1 || betType > 6) {
        printf(RED "Invalid bet type! Please enter a number between 1 and 6.\n" RESET);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    switch (betType) {
        case 1:
            printWheel();
            printf(BOLD BLUE "Enter the number you want to bet on (0-36): " RESET);
            if (scanf("%d", &betNum) != 1 || betNum < 0 || betNum > 36) {
                printf(RED "Invalid number! Please enter a number between 0 and 36.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
        case 2:
            printf(BOLD BLUE "1) Even 2) Odd: " RESET);
            if (scanf("%d", &betNum) != 1 || (betNum != 1 && betNum != 2)) {
                printf(RED "Invalid choice! Please select 1 for Even or 2 for Odd.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
        case 3:
            printf(BOLD BLUE "1) Red 2) Black: " RESET);
            if (scanf("%d", &betNum) != 1 || (betNum != 1 && betNum != 2)) {
                printf(RED "Invalid choice! Please select 1 for Red or 2 for Black.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
        case 4:
            printf(BOLD BLUE "1) 1-18 2) 19-36: " RESET);
            if (scanf("%d", &betNum) != 1 || (betNum != 1 && betNum != 2)) {
                printf(RED "Invalid choice! Please select 1 for 1-18 or 2 for 19-36.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
        case 5:
            printf(BOLD BLUE "1) 1st Dozen (1-12)\n2) 2nd Dozen (13-24)\n3) 3rd Dozen (25-36)\nEnter your dozen selection: " RESET);
            if (scanf("%d", &betNum) != 1 || betNum < 1 || betNum > 3) {
                printf(RED "Invalid dozen selection! Please enter 1, 2, or 3.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
        case 6:
            printf(BOLD BLUE "1) 1st Column (1,4,7...)\n2) 2nd Column (2,5,8...)\n3) 3rd Column (3,6,9...)\nEnter your column selection: " RESET);
            if (scanf("%d", &betNum) != 1 || betNum < 1 || betNum > 3) {
                printf(RED "Invalid column selection! Please enter 1, 2, or 3.\n" RESET);
                clearInputBuffer();
                return;
            }
            break;
    }
    clearInputBuffer();

    printf(BOLD YELLOW "Enter your bet amount ($10-$1000): " RESET);
    if (scanf("%d", &betAmt) != 1 || betAmt < 10 || betAmt > 1000 || betAmt > user->balance) {
        printf(RED "Invalid bet amount! Must be between $10 and $1000 and not exceed your balance.\n" RESET);
        clearInputBuffer();
        return;
    }
    clearInputBuffer();

    user->balance -= betAmt;
    user->games_played++;
    
    int result;
    char color[20];
    spinRoulette(&result, color);
    printf(CYAN "\nSpinning the wheel...\n" RESET);
    printf(BOLD WHITE "Ball lands on: " MAGENTA "%d (%s)\n" RESET, result, color);

    int won = 0;
    
    bool isRedResult = false;
    if (result != 0) {
        for (int i = 0; i < sizeof(redNumbers)/sizeof(redNumbers[0]); i++) {
            if (result == redNumbers[i]) {
                isRedResult = true;
                break;
            }
        }
    }

    switch (betType) {
        case 1: if (betNum == result) { won = 35; strcpy(gameType, "Single Number"); } break;
        case 2: 
            if (result != 0) {
                if ((betNum == 1 && result % 2 == 0) || (betNum == 2 && result % 2 != 0)) {
                    won = 1; strcpy(gameType, (betNum == 1) ? "Even" : "Odd");
                }
            }
            break;
        case 3: 
            if (result != 0) {
                if ((betNum == 1 && isRedResult) || (betNum == 2 && !isRedResult)) {
                    won = 1; strcpy(gameType, (betNum == 1) ? "Red" : "Black");
                }
            }
            break;
        case 4: 
            if (result != 0) {
                if ((betNum == 1 && result >= 1 && result <= 18) || (betNum == 2 && result >= 19 && result <= 36)) {
                    won = 1; strcpy(gameType, (betNum == 1) ? "Low (1-18)" : "High (19-36)");
                }
            }
            break;
        case 5: 
            if (result != 0) {
                 if ((betNum == 1 && result >= 1 && result <= 12) ||
                    (betNum == 2 && result >= 13 && result <= 24) ||
                    (betNum == 3 && result >= 25 && result <= 36)) {
                    won = 2; snprintf(gameType, sizeof(gameType), "Dozens %d-%d", (betNum-1)*12+1, betNum*12);
                }
            }
            break;
        case 6: 
            if (result != 0) {
                if ((betNum == 1 && result % 3 == 1) || (betNum == 2 && result % 3 == 2) || (betNum == 3 && result % 3 == 0)) {
                    won = 2; snprintf(gameType, sizeof(gameType), "Column %d", betNum);
                }
            }
            break;
    }

    int payout = won * betAmt;
    if (won) {
        user->balance += payout + betAmt;
        user->games_won++;
        if (payout > user->highest_win) user->highest_win = payout;
        printf(GREEN BOLD "\n*** YOU WON $%d! Your new balance: $%d ***\n" RESET, payout, user->balance);
    } else {
        printf(RED BOLD "\n--- YOU LOST $%d. Your new balance: $%d ---\n" RESET, betAmt, user->balance);
    }

    addGameHistory(*user, gameType, betAmt, result, payout);
    updateUser(*user);
}

void showGameHistory(const char* username) {
    printf(BOLD CYAN "\n====== Your Game History ======\n" RESET);
    printf(YELLOW "%-20s %-15s %-8s %-5s %s\n" RESET, "Time", "Type", "Bet", "Result", "Payout");
    int found = 0;
    for (int i = historyCount-1; i >= 0; i--) {
        if (strcmp(gameHistory[i].username, username) == 0) {
            printf(WHITE "%-20s %-15s $" GREEN "%-7d" RESET " %-5d $" MAGENTA "%d\n" RESET, 
                     gameHistory[i].timestamp,
                     gameHistory[i].game_type, 
                     gameHistory[i].bet_amount,
                     gameHistory[i].result, 
                     gameHistory[i].payout);
            found = 1;
        }
    }
    if (!found) printf(YELLOW "No game history found for %s.\n" RESET, username);
    printf(BOLD CYAN "===============================\n" RESET);
}

void displayStats(User user) {
    float winRate = user.games_played ? 
                    (float)user.games_won / user.games_played * 100 : 0;
    printf(BOLD MAGENTA "\n=== Your Statistics ===\n" RESET
           WHITE "Username: %s\n"
           "Current Balance: $" GREEN "%d\n" RESET
           WHITE "Games Played: %d\n"
           "Games Won: %d\n"
           "Win Percentage: " YELLOW "%.1f%%\n" RESET
           WHITE "Highest Single Win: $" MAGENTA "%d\n" RESET
           BOLD MAGENTA "=======================\n" RESET, 
           user.username, user.balance, user.games_played,
           user.games_won, winRate, user.highest_win);
}

void changePassword(User *user) {
    char current[PASSWORD_LENGTH], newPass[PASSWORD_LENGTH], confirm[PASSWORD_LENGTH];
    
    printf(BOLD YELLOW "\n--- Change Password ---\n" RESET);
    printf(CYAN "Enter your current password: " RESET);
    getInput(current, sizeof(current));
    
    if (!verifyPassword(current, user->password)) {
        printf(RED BOLD "Incorrect current password! Password change failed.\n" RESET);
        return;
    }
    
    printf(CYAN "Enter new password (minimum 6 characters): " RESET);
    getInput(newPass, sizeof(newPass));
    
    if (strlen(newPass) < 6) {
        printf(RED BOLD "Password too short! Minimum 6 characters required.\n" RESET);
        return;
    }
    
    printf(CYAN "Confirm new password: " RESET);
    getInput(confirm, sizeof(confirm));
    
    if (strcmp(newPass, confirm) != 0) {
        printf(RED BOLD "Passwords do not match! Password change failed.\n" RESET);
        return;
    }
    
    encryptPassword(newPass); 
    strcpy(user->password, newPass);
    updateUser(*user);
    printf(GREEN BOLD "Password changed successfully!\n" RESET);
}

void checkTimeout(time_t lastActivity) {
    if (difftime(time(NULL), lastActivity) > INACTIVITY_TIMEOUT) {
        printf(YELLOW BOLD "\nSession expired due to inactivity. Logging out...\n" RESET);
        exit(0);
    }
}

void adminMenu(User *currentAdmin) {
    if (!currentAdmin->isAdmin) {
        printf(RED BOLD "\nAdmin privileges required to access this menu!\n" RESET);
        return;
    }

    int choice;
    char targetUsername[50];
    User users[MAX_USERS];
    
    while (1) {
        printf(BOLD BLUE "\n|=======================|\n");
        printf("|    ADMIN PANEL        |\n");
        printf("|=======================|\n" RESET);
        printf(WHITE "1) Reset user balance\n"
               "2) View all users\n"
               "3) Promote to admin\n"
               RED "4) Return to Main Menu\n" RESET
               BOLD CYAN "Enter your choice: " RESET);

        if (scanf("%d", &choice) != 1) {
            printf(RED BOLD "Invalid input! Please enter a number.\n" RESET);
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: {
                printf(CYAN "Enter username to reset balance for: " RESET);
                getInput(targetUsername, sizeof(targetUsername));

                FILE *file = fopen(FILENAME, "r");
                if (!file) {
                    printf(RED BOLD "Error opening user database!\n" RESET);
                    break;
                }

                int count = 0;
                int found = 0;
                int adminFlag;
                char encrypted_pw_from_file[PASSWORD_LENGTH];

                while (fscanf(file, "%49s %19s %d %d %d %d %d", 
                                users[count].username, encrypted_pw_from_file,
                                &users[count].balance, &users[count].games_played,
                                &users[count].games_won, &users[count].highest_win,
                                &adminFlag) == 7) {
                    users[count].isAdmin = (adminFlag == 1);
                    strcpy(users[count].password, encrypted_pw_from_file);
                    
                    if (strcmp(users[count].username, targetUsername) == 0) {
                        users[count].balance = STARTING_BALANCE;
                        found = 1;
                    }
                    count++;
                }
                fclose(file);

                if (!found) {
                    printf(RED BOLD "User '%s' not found!\n" RESET, targetUsername);
                    break;
                }

                file = fopen(FILENAME, "w");
                if (!file) {
                    printf(RED BOLD "Error saving changes!\n" RESET);
                    break;
                }

                for (int i = 0; i < count; i++) {
                    fprintf(file, "%s %s %d %d %d %d %d\n", 
                            users[i].username, users[i].password,
                            users[i].balance, users[i].games_played,
                            users[i].games_won, users[i].highest_win,
                            users[i].isAdmin ? 1 : 0);
                }
                fclose(file);

                printf(GREEN BOLD "Successfully reset %s's balance to $%d\n" RESET, 
                            targetUsername, STARTING_BALANCE);
                break;
            }

            case 2: {
                FILE *file = fopen(FILENAME, "r");
                if (!file) {
                    printf(RED BOLD "Error opening user database!\n" RESET);
                    break;
                }

                printf(BOLD YELLOW "\n|==================|==========|=======|=======|\n");
                printf("| %-16s | %-8s | %-5s | Admin |\n", "Username", "Balance", "Games");
                printf("|==================|==========|=======|=======|\n" RESET);

                User user_read;
                char encrypted_pw_from_file[PASSWORD_LENGTH];
                int adminFlag;
                
                while (fscanf(file, "%49s %19s %d %d %d %d %d", 
                                user_read.username, encrypted_pw_from_file,
                                &user_read.balance, &user_read.games_played,
                                &user_read.games_won, &user_read.highest_win,
                                &adminFlag) == 7) {
                    printf(WHITE "| %-16s | $" GREEN "%-7d" RESET WHITE " | %-5d | %-5s |\n" RESET, 
                                user_read.username, user_read.balance,
                                user_read.games_played, 
                                adminFlag ? GREEN "Yes" : RED "No"); // Color for Admin status
                }
                fclose(file);

                printf(BOLD YELLOW "|==================|==========|=======|=======|\n" RESET);
                break;
            }

            case 3: {
                printf(CYAN "Enter username to promote to admin: " RESET);
                getInput(targetUsername, sizeof(targetUsername));

                FILE *file = fopen(FILENAME, "r");
                if (!file) {
                    printf(RED BOLD "Error opening user database!\n" RESET);
                    break;
                }

                int count = 0;
                int found = 0;
                int adminFlag;
                char encrypted_pw_from_file[PASSWORD_LENGTH];

                while (fscanf(file, "%49s %19s %d %d %d %d %d", 
                                users[count].username, encrypted_pw_from_file,
                                &users[count].balance, &users[count].games_played,
                                &users[count].games_won, &users[count].highest_win,
                                &adminFlag) == 7) {
                    users[count].isAdmin = (adminFlag == 1);
                    strcpy(users[count].password, encrypted_pw_from_file);
                    
                    if (strcmp(users[count].username, targetUsername) == 0) {
                        users[count].isAdmin = 1;
                        found = 1;
                    }
                    count++;
                }
                fclose(file);

                if (!found) {
                    printf(RED BOLD "User '%s' not found!\n" RESET, targetUsername);
                    break;
                }

                file = fopen(FILENAME, "w");
                if (!file) {
                    printf(RED BOLD "Error saving changes!\n" RESET);
                    break;
                }

                for (int i = 0; i < count; i++) {
                    fprintf(file, "%s %s %d %d %d %d %d\n", 
                            users[i].username, users[i].password,
                            users[i].balance, users[i].games_played,
                            users[i].games_won, users[i].highest_win,
                            users[i].isAdmin ? 1 : 0);
                }
                fclose(file);

                printf(GREEN BOLD "%s is now an admin!\n" RESET, targetUsername);
                break;
            }

            case 4:
                printf(YELLOW "Returning to main menu...\n" RESET);
                return;

            default:
                printf(RED BOLD "Invalid choice! Please select 1-4.\n" RESET);
        }
    }
}

int main() {
    srand(time(NULL));
    User user;
    int choice;
    time_t lastActivity = time(NULL);

    memset(&user, 0, sizeof(User));

    printf(BOLD CYAN "====== Welcome To Team 16 Roulette Casino ======\n" RESET);
    printf(BOLD YELLOW "1) Login\n2) Register\n" RED "3) Exit\n" RESET
           BOLD CYAN "Enter your choice: " RESET);
    if (scanf("%d", &choice) != 1) {
        printf(RED BOLD "Invalid choice! Please enter a number.\n" RESET);
        clearInputBuffer();
        return 1;
    }
    clearInputBuffer();

    if (choice == 3) {
        printf(YELLOW "Exiting program. Goodbye!\n" RESET);
        return 0;
    }

    printf(BOLD BLUE "Username: " RESET);
    getInput(user.username, sizeof(user.username));

    if (choice == 1) { // Login
        printf(BOLD BLUE "Password: " RESET);
        getInput(user.password, sizeof(user.password));
        
        int found = loadUser(&user);
        
        if (found == -1) {
            printf(RED BOLD "Incorrect password!\n" RESET);
            return 1;
        } else if (!found) {
            printf(RED BOLD "User '%s' not found! Please register.\n" RESET, user.username);
            return 1;
        }

        printf(GREEN BOLD "\nWelcome back, %s! Enjoy the game!\n" RESET, user.username);

    } else if (choice == 2) { // Register
        User temp_check_user;
        strcpy(temp_check_user.username, user.username);
        temp_check_user.password[0] = '\0'; 
        if (loadUser(&temp_check_user)) {
            printf(RED BOLD "User '%s' already exists! Please login instead.\n" RESET, user.username);
            return 1;
        }

        printf(BOLD BLUE "Enter Password (min 6 characters): " RESET);
        getInput(user.password, sizeof(user.password));
        
        if (strlen(user.password) < 6) {
            printf(RED BOLD "Password too short! Minimum 6 characters required.\n" RESET);
            return 1;
        }
        
        user.isAdmin = 0;
        if (strcmp(user.username, "admin") == 0) {
            printf(YELLOW "You are registering as 'admin'. Enter admin activation code: " RESET);
            char code[50];
            getInput(code, sizeof(code));
            if (isAdminPassword(code)) {
                user.isAdmin = 1;
                printf(GREEN BOLD "Admin privileges granted upon registration!\n" RESET);
            } else {
                printf(YELLOW "Incorrect admin code. Registering as a standard account.\n" RESET);
            }
        }
        
        user.balance = STARTING_BALANCE;
        user.games_played = 0;
        user.games_won = 0;
        user.highest_win = 0;
        
        encryptPassword(user.password);
        saveUser(user);
        printf(GREEN BOLD "\nRegistered successfully! Starting balance: $" CYAN "%d\n" RESET, STARTING_BALANCE);
    } else {
        printf(RED BOLD "Invalid initial choice! Please select 1, 2, or 3.\n" RESET);
        return 1;
    }

    displayRules();

    while (1) {
        checkTimeout(lastActivity);
        lastActivity = time(NULL);

        printf(BOLD CYAN "\n====== Main Menu ======\n" RESET);
        printf(YELLOW "1) Play Roulette\n"
               "2) View Balance\n"
               "3) View Statistics\n"
               "4) View Game History\n"
               "5) Change Password\n" RESET);
        if (user.isAdmin) printf(MAGENTA "6) Admin Menu\n" RESET);
        printf(RED "0) Exit\n" RESET BOLD CYAN "Enter your choice: " RESET);

        if (scanf("%d", &choice) != 1) {
            printf(RED BOLD "Invalid choice! Please enter a number.\n" RESET);
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: playRoulette(&user); break;
            case 2: printf(WHITE "\nYour Current Balance: $" GREEN "%d\n" RESET, user.balance); break;
            case 3: displayStats(user); break;
            case 4: showGameHistory(user.username); break;
            case 5: changePassword(&user); break;
            case 6: 
                if (user.isAdmin) {
                    adminMenu(&user); 
                } else {
                    printf(RED BOLD "Admin privileges required for this option.\n" RESET);
                }
                break;
            case 0: 
                printf(GREEN BOLD "\nThank you for playing, %s! Your final balance: $" CYAN "%d\n" RESET, 
                       user.username, user.balance);
                return 0;
            default: printf(RED BOLD "Invalid choice! Please select a valid option from the menu.\n" RESET);
        }
    }
    return 0;
}