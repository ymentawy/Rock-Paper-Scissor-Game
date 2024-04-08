/*
 * Application.h
 *
 *  Created on: Dec 29, 2019
 *      Author: Matthew Zhong
 *  Supervisor: Leyla Nazhand-Ali
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <HAL/HAL.h>

// Maximum length for text
#define MAX_TEXT_LENGTH 100

// Maximum length for strings
#define MAX_STRING_LENGTH 21

// Maximum length for player names
#define MAX_NAME_LENGTH 4

// Length for player choices
#define CHOICE_LENGTH 2

// Maximum number of lines
#define MAX_LINES 16

// Increment value for Y position
#define Y_INCREMENTAL 8

// Maximum number of rounds
#define MAX_ROUNDS 7

// Minimum number of rounds
#define MIN_ROUNDS 1

// Default number of rounds
#define DEF_ROUNDS 3

// Maximum number of players
#define MAX_PLAYERS 5

// Minimum number of players
#define MIN_PLAYERS 2

// Default number of players
#define DEF_PLAYERS 2

// Position for rounds display
#define ROUNDS_POS 56

// Position for players display
#define PLAYERS_POS 72

// Structure for the Application object
struct _Application {
  // Put your application members and FSM state variables here!
  // =========================================================================
  UART_Baudrate baudChoice; // Selected baud rate
  bool firstCall; // Flag for first call to application
  screen screen_state; // Enumeration for different screen states
  int players; // Number of players
  int rounds; // Number of rounds
  bool toggle_players; // Toggle flag for players
  bool players_done; // Flag indicating if all players are done
  char names[MAX_PLAYERS-1][MAX_NAME_LENGTH]; // Array to store player names
  char choices[MAX_PLAYERS - 1][CHOICE_LENGTH]; // Array to store player choices
  int players_count; // Count of players
  int rounds_count; // Count of rounds
  int wins[MAX_PLAYERS - 1]; // Array to store player wins
  bool end;
};
typedef struct _Application Application;

// Constructor for the Application object
Application Application_construct();

// Main super-loop function
void Application_loop(Application* app, HAL* hal);

// Updates communications settings
void Application_updateCommunications(Application* app, HAL* hal);

// Interprets incoming character and echoes back to terminal what kind of character was received
char Application_interpretIncomingChar(char);

// Generic circular increment function
uint32_t CircularIncrement(uint32_t value, uint32_t maximum);

// Finite state machine for game
void Game_FSM(Application* app_p, HAL* hal_p);

// Function declarations for printing different screens
void print_title(Application* app_p, HAL* hal_p);
void print_instructions(Application* app_p, HAL* hal_p);
void print_settings(Application* app_p, HAL* hal_p, bool PR, bool rst);
void print_selection(Application* app_p, HAL* hal_p);
void print_game(Application* app_p, HAL* hal_p);
void print_over(Application* app_p, HAL* hal_p);
void clear_screen(HAL* hal_p);
void PR_Toggle(int *currentNum);
void PlayerIncrement(int *currentNum);
void RoundIncrement(int *currentNum);
void Toggle(int* astr_y, int* space_y, int* players, int* rounds, bool PR, bool rst);
void uart_print(Application* app_p, HAL* hal_p);
void uart_name(HAL* hal_p, char* name);
void uart_new_line(HAL* hal_p);
void invalid_input(HAL* hal_p);
void game_round(Application* app_p, HAL* hal_p);
void print_scores(Application* app_p, HAL* hal_p);
void print_BB1(Application* app_p, HAL* hal_p);
void print_BB1_end(Application* app_p, HAL* hal_p);
void print_BB1_end_screen(Application* app_p, HAL* hal_p);
void determine_winners(Application* app_p);
void wins_rst(Application* app_p);
void title_screen_state(Application* app_p, HAL* hal_p);
void instructions_screen_state(Application* app_p, HAL* hal_p);
void settings_screen_state(Application* app_p, HAL* hal_p);
void selection_screen_state(Application* app_p, HAL* hal_p);
void game_screen_state(Application* app_p, HAL* hal_p);
void end_screen_state(Application* app_p, HAL* hal_p);

#endif /* APPLICATION_H_ */
