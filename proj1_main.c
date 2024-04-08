/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* HAL and Application includes */
#include <Application.h>
#include <HAL/HAL.h>
#include <HAL/Timer.h>

// Define a maximum length for the concatenated string



// Non-blocking check. Whenever Launchpad S1 is pressed, LED1 turns on.
static void InitNonBlockingLED() {
  GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0);
  GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
}

// Non-blocking check. Whenever Launchpad S1 is pressed, LED1 turns on.
static void PollNonBlockingLED() {
  GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0);
  if (GPIO_getInputPinValue(GPIO_PORT_P1, GPIO_PIN1) == 0) {
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
  }
}

/**
 * The main entry point of your project. The main function should immediately
 * stop the Watchdog timer, call the Application constructor, and then
 * repeatedly call the main super-loop function. The Application constructor
 * should be responsible for initializing all hardware components as well as all
 * other finite state machines you choose to use in this project.
 *
 * THIS FUNCTION IS ALREADY COMPLETE. Unless you want to temporarily experiment
 * with some behavior of a code snippet you may have, we DO NOT RECOMMEND
 * modifying this function in any way.
 */
int main(void) {
  // Stop Watchdog Timer - THIS SHOULD ALWAYS BE THE FIRST LINE OF YOUR MAIN
  WDT_A_holdTimer();

  // Initialize the system clock and background hardware timer, used to enable
  // software timers to time their measurements properly.
  InitSystemTiming();

  // Initialize the main Application object and HAL object
  HAL hal = HAL_construct();
  Application app = Application_construct();

  // Do not remove this line. This is your non-blocking check.
  InitNonBlockingLED();

  // Main super-loop! In a polling architecture, this function should call
  // your main FSM function over and over.
  while (true) {
    // Do not remove this line. This is your non-blocking check.
    PollNonBlockingLED();
    HAL_refresh(&hal);
    Application_loop(&app, &hal);
  }
}

/**
 * A helper function which increments a value with a maximum. If incrementing
 * the number causes the value to hit its maximum, the number wraps around
 * to 0.
 */
uint32_t CircularIncrement(uint32_t value, uint32_t maximum) {
  return (value + 1) % maximum;
}

/**
 * The main constructor for your application. This function should initialize
 * each of the FSMs which implement the application logic of your project.
 *
 * @return a completely initialized Application object
 */
Application Application_construct() {
  Application app;

  // Initialize local application state variables here!
  app.baudChoice = BAUD_9600;
  app.firstCall = true;
  app.screen_state = title;
  app.players = DEF_PLAYERS;
  app.rounds = DEF_ROUNDS;
  app.toggle_players = false;
  app.players_count = 0;
  app.rounds_count = 0;
  app.players_done = false;
  app.end = false;

  return app;
}

/*
 * Application_loop
 *
 * This function is called once per super-loop of the main application. It manages the main
 * application logic, including updating communications, handling button presses, and printing
 * output via UART.
 *
 * Parameters:
 *   - app_p: Pointer to the Application struct containing application state and variables.
 *   - hal_p: Pointer to the HAL struct containing hardware abstraction layer functions and variables.
 *
 * Description:
 *   - Restarts or updates communications if this is the first time the application is run or if
 *     BoosterPack S2 is pressed (which indicates a new baudrate is being set up).
 *   - Calls the Game_FSM function to manage the game's finite state machine.
 *   - Checks if BoosterPack S2 is tapped or if it's the first call to update communications accordingly.
 *   - Prints output via UART.
 */
void Application_loop(Application* app_p, HAL* hal_p) {
  // Restart/Update communications if either this is the first time the
  // application is run or if BoosterPack S2 is pressed (which means a new
  // baudrate is being set up)

  Game_FSM(app_p, hal_p);

  if (Button_isTapped(&hal_p->boosterpackS2) || app_p->firstCall) {
    Application_updateCommunications(app_p, hal_p);
  }

  uart_print(app_p, hal_p);
}


/**
 * Updates which LEDs are lit and what baud rate the UART module communicates
 * with, based on what the application's baud choice is at the time this
 * function is called.
 *
 * @param app_p:  A pointer to the main Application object.
 * @param hal_p:  A pointer to the main HAL object
 */
void Application_updateCommunications(Application* app_p, HAL* hal_p) {
  // When this application first loops, the proper LEDs aren't lit. The
  // firstCall flag is used to ensure that the
  if (app_p->firstCall) {
    app_p->firstCall = false;
  }

  // When BoosterPack S2 is tapped, circularly increment which baud rate is
  // used.
  else {
    uint32_t newBaudNumber =
        CircularIncrement((uint32_t)app_p->baudChoice, NUM_BAUD_CHOICES);
    app_p->baudChoice = (UART_Baudrate)newBaudNumber;
  }

  // Start/update the baud rate according to the one set above.
  UART_SetBaud_Enable(&hal_p->uart, app_p->baudChoice);

  // Based on the new application choice, turn on the correct LED.
  // To make your life easier, we recommend turning off all LEDs before
  // selectively turning back on only the LEDs that need to be relit.
  // -------------------------------------------------------------------------
  LED_turnOff(&hal_p->launchpadLED2Red);
  LED_turnOff(&hal_p->launchpadLED2Green);
  LED_turnOff(&hal_p->launchpadLED2Blue);

  switch (app_p->baudChoice) {
    // When the baud rate is 9600, turn on Launchpad LED Red
    case BAUD_9600:
      LED_turnOn(&hal_p->launchpadLED2Red);
      break;

    case BAUD_19200:
      LED_turnOn(&hal_p->launchpadLED2Green);
      break;

    case BAUD_38400:
      LED_turnOn(&hal_p->launchpadLED2Blue);
      break;

    case BAUD_57600:
      LED_turnOn(&hal_p->launchpadLED2Red);
      LED_turnOn(&hal_p->launchpadLED2Green);
      LED_turnOn(&hal_p->launchpadLED2Blue);
      break;

    // In the default case, this program will do nothing.
    default:
      break;
  }
}

/**
 * Interprets a character which was incoming and returns an interpretation of
 * that character. If the input character is a letter, it return L for Letter,
 * if a number return N for Number, and if something else, it return O for
 * Other.
 *
 * @param rxChar: Input character
 * @return :  Output character
 */
char Application_interpretIncomingChar(char rxChar) {
  // The character to return back to sender. By default, we assume the letter
  // to send back is an 'O' (assume the character is an "other" character)
  //char txChar = 'O';
  char txChar;

  // Numbers - if the character entered was a number, transfer back an 'N'
  if (rxChar >= '0' && rxChar <= '9') {
    txChar = rxChar;
  }
  // Letters - if the character entered was a letter, transfer back an 'L'
  else if ((rxChar >= 'a' && rxChar <= 'z') || (rxChar >= 'A' && rxChar <= 'Z')) {
    txChar = rxChar;
  }
  else {
      return '\0';
  }

  return (txChar);
}

void uart_print(Application* app_p, HAL* hal_p) {
    // Static variable to keep track of the current player index
    static int i = 0;

    // Check if conditions for UART processing are met
    if ((app_p->screen_state == name_selection && i < app_p->players && app_p->toggle_players)) {
        // Check if there's a character available from the UART
        if (UART_hasChar(&hal_p->uart)) {
            // The character received from the serial terminal
            char rxChar = UART_getChar(&hal_p->uart);

            // Interpret the incoming character
            char txChar = Application_interpretIncomingChar(rxChar);

            // Proceed if the interpreted character is not null
            if (txChar != '\0') {
                // Control the blue LED based on the received character
                if (rxChar == 'r' || rxChar == 'p' || rxChar == 's') {
                    LED_turnOn(&hal_p->boosterpackBlue);
                } else {
                    LED_turnOff(&hal_p->boosterpackBlue);
                }

                // Concatenate the received character to the player's name if it's not at maximum length
                if (strlen(app_p->names[i]) < MAX_NAME_LENGTH - 1) {
                    strcat(app_p->names[i], &txChar);

                    // Send the character back through UART if transmission is possible
                    if (UART_canSend(&hal_p->uart)) {
                        UART_sendChar(&hal_p->uart, txChar);
                    }

                    // Update the graphics context to display the updated name on the screen
                    Graphics_drawString(&hal_p->g_sContext, (int8_t *)app_p->names[i], -1, 25, (i + 1) * 8, true);

                    // If the name reaches maximum length, disable player toggle and move to the next player
                    if (strlen(app_p->names[i]) == MAX_NAME_LENGTH - 1) {
                        app_p->toggle_players = false;
                        i++;
                    }
                }
            }
        }
    }
}


// Function for handling the title screen state
void title_screen_state(Application* app_p, HAL* hal_p){
    // Check if boosterpackS1 button is tapped
    if (Button_isTapped(&hal_p->boosterpackS1)){
        // Transition to settings screen
        app_p->screen_state = settings;
        // Clear the screen
        clear_screen(hal_p);
        // Print settings
        print_settings(app_p, hal_p, true, false);
    }
    // Check if launchpadS2 button is tapped
    else if (Button_isTapped(&hal_p->launchpadS2)){
        // Transition to instructions screen
        app_p->screen_state = instructions;
        // Clear the screen
        clear_screen(hal_p);
        // Print instructions
        print_instructions(app_p, hal_p);
    }
    else {
        // Print title
        print_title(app_p, hal_p);
    }
}

// Function for handling the instructions screen state
void instructions_screen_state(Application* app_p, HAL* hal_p){
    // Check if launchpadS2 button is tapped
    if (Button_isTapped(&hal_p->launchpadS2)){
        // Transition to title screen
        app_p->screen_state = title;
        // Clear the screen
        clear_screen(hal_p);
        // Print title
        print_title(app_p, hal_p);
    }
}

// Function for handling the settings screen state
void settings_screen_state(Application* app_p, HAL* hal_p){
    // Check if boosterpackS1 button is tapped
    if (Button_isTapped(&hal_p->boosterpackS1)){
        // Transition to name selection screen
        app_p->screen_state = name_selection;
        // Clear the screen
        clear_screen(hal_p);
        // Print selection
        print_selection(app_p, hal_p);
        // Enable player toggle
        app_p->toggle_players = true;
        // Flush UART buffer
        if (UART_hasChar(&hal_p->uart))
            UART_getChar(&hal_p->uart);
    }
    // Check if launchpadS2 button is tapped
    else if (Button_isTapped(&hal_p->launchpadS2)){
        // Print settings with default options
        print_settings(app_p, hal_p, true, false);
    }
    // Check if boosterpackJS button is tapped
    else if (Button_isTapped(&hal_p->boosterpackJS)){
        // Print settings with joystick enabled
        print_settings(app_p, hal_p, false, false);
    }
    // Check if launchpadS1 button is tapped
    else if (Button_isTapped(&hal_p->launchpadS1)){
        // Print settings with sound enabled
        print_settings(app_p, hal_p, false, true);
    }
}

// Function for handling the selection screen state
void selection_screen_state(Application* app_p, HAL* hal_p){
    // Check if boosterpackS1 button is tapped and conditions for player selection are met
    if (Button_isTapped(&hal_p->boosterpackS1) && ((app_p->players_count == app_p->players) || (app_p->players_count == app_p->players - 1 && strlen(app_p->names[app_p->players_count]) == MAX_NAME_LENGTH - 1))){
        // Null-terminate the current player's name
        app_p->names[app_p->players_count][MAX_NAME_LENGTH - 1] = '\0';
        // Flush UART buffer
        if (UART_hasChar(&hal_p->uart))
            UART_getChar(&hal_p->uart);
        // Move to game screen state
        app_p->screen_state = game;
        // Reset wins count
        wins_rst(app_p);
        // Clear the screen
        clear_screen(hal_p);
        // Reset players count
        app_p->players_count = 0;
        // Print game screen
        print_game(app_p, hal_p);
        // Start game round
        game_round(app_p, hal_p);
        // Print newline through UART
        uart_new_line(hal_p);
    }
    // Check if boosterpackS1 button is tapped and there are still players to be selected
    else if (Button_isTapped(&hal_p->boosterpackS1) && (app_p->players_count < app_p->players)){
        // Check if the current player's name length has reached its maximum
        if (strlen(app_p->names[app_p->players_count]) == MAX_NAME_LENGTH - 1){
            // Enable player toggle and print selection
            app_p->toggle_players = true;
            print_selection(app_p, hal_p);
            // Null-terminate the current player's name
            app_p->names[app_p->players_count][MAX_NAME_LENGTH - 1] = '\0';
            // Flush UART buffer
            if (UART_hasChar(&hal_p->uart))
                UART_getChar(&hal_p->uart);
            // Increment players count
            app_p->players_count++;
            // Clear current player's name if not empty
            if (strlen(app_p->names[app_p->players_count]) != 0)
                strcpy(app_p->names[app_p->players_count], '\0');
            // Print newline through UART
            uart_new_line(hal_p);
        }
    }
}

// Function for handling the game screen state
void game_screen_state(Application* app_p, HAL* hal_p){
    // Check if boosterpackS1 button is tapped and the maximum number of rounds has not been reached
    if ((Button_isTapped(&hal_p->boosterpackS1) && app_p->rounds_count <= app_p->rounds)){
        // Print scores and start new round
        print_scores(app_p, hal_p);
        if (UART_hasChar(&hal_p->uart))
            UART_getChar(&hal_p->uart);
        game_round(app_p, hal_p);
        app_p->players_done = false;
        // Check if all rounds have been played
        if (app_p->rounds_count == app_p->rounds){
            // Transition to game over screen
            app_p->screen_state = game_over;
            // Print end screen on boosterpack1
            print_BB1_end_screen(app_p, hal_p);
            // Print game over message
            print_BB1_end(app_p, hal_p);
        }
    }
    // If not all players have finished their turns, continue game round
    else if (!app_p->players_done)
        game_round(app_p, hal_p);
}

// Function for handling the end screen state
void end_screen_state(Application* app_p, HAL* hal_p){
    // Check if boosterpackS1 button is tapped and end flag is not set
    if (Button_isTapped(&hal_p->boosterpackS1) && !app_p->end){
        // Clear the screen and print game over message
        clear_screen(hal_p);
        print_over(app_p, hal_p);
        // Set end flag
        app_p->end = true;
    }
    // If end flag is set, return
    else if (app_p->end)
        return;
}


// Function for managing the game finite state machine
void Game_FSM(Application* app_p, HAL* hal_p) {
    // Switch based on the current screen state
    switch (app_p->screen_state) {
        case title:
            // Call function for title screen state
            title_screen_state(app_p, hal_p);
            break;

        case instructions:
            // Call function for instructions screen state
            instructions_screen_state(app_p, hal_p);
            break;

        case settings:
            // Call function for settings screen state
            settings_screen_state(app_p, hal_p);
            break;

        case name_selection:
            // Call function for name selection screen state
            selection_screen_state(app_p, hal_p);
            break;

        case game:
            // Call function for game screen state
            game_screen_state(app_p, hal_p);
            break;

        case game_over:
            // Call function for end screen state
            end_screen_state(app_p, hal_p);
            break;
    }
}

// Function for sending a new line over UART
void uart_new_line(HAL* hal_p){
    // Check if UART can send characters
    if (UART_canSend(&hal_p->uart)){
        // Send carriage return and line feed characters
        UART_sendChar(&hal_p->uart, '\r');
        UART_sendChar(&hal_p->uart, '\n');
    }
}

// Function for handling a single round of the game
void game_round(Application* app_p, HAL* hal_p){
    // Static variables to maintain state across function calls
    static bool err = false;
    static char rxChar;

    // Check if the current round is less than the total rounds
    if (app_p -> rounds_count < app_p -> rounds){
        // Check if there is no error
        if (!err)
            // Send the player's name over UART
            uart_name(hal_p, app_p->names[app_p -> players_count]);

        // Check if UART has received a character
        if (UART_hasChar(&hal_p->uart))
            // Get the received character
            rxChar = UART_getChar(&hal_p->uart);

        // Check if the received character is a valid choice
        if (rxChar == 'r' || rxChar == 'p' || rxChar == 's'){
            // Reset error flag
            err = false;
            // Record the player's choice
            app_p->choices[app_p -> players_count][0] = rxChar;
            app_p->choices[app_p -> players_count][1] = '\0';
            rxChar = '\0';
            // Increment the player count
            app_p -> players_count++;
            // Check if all players have made their choices
            if (app_p -> players_count == app_p -> players){
                // Increment the round count
                app_p -> rounds_count++;
                // Determine the winners
                determine_winners(app_p);
                // Reset player count and set players_done flag
                app_p -> players_count = 0;
                app_p -> players_done = true;
                // Print game state
                print_BB1(app_p, hal_p);
            }
            else
                app_p -> players_done = false;
        }
        // Check if the received character is null
        else if (rxChar == '\0')
            err = true;
        else{
            // Reset received character and flag error
            rxChar = '\0';
            // Print invalid input message
            invalid_input(hal_p);
            err = true;
        }
    }
}

// Function to print the title screen
void print_title(Application* app_p, HAL* hal_p){

    // Static array to store lines of text
    static char lines[MAX_LINES][MAX_STRING_LENGTH];

    // Copy and draw the title text
    strcpy(lines[2], "Rock Paper Scissors");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[2], -1, 0, 16, true);

    // Copy and draw the subtitle text
    strcpy(lines[3], "Multiplayer Game");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[3], -1, 0, 24, true);

    // Copy and draw additional text lines
    strcpy(lines[4], "My solution");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[4], -1, 0, 32, true);
    strcpy(lines[6], "Youssef Mentawy");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[6], -1, 0, 48, true);
    strcpy(lines[10], "BB1: Play Game");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[10], -1, 0, 80, true);
    strcpy(lines[11], "LB2: Instructions");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[11], -1, 0, 88, true);
}

// Function to print the instructions screen
void print_instructions(Application* app_p, HAL* hal_p){
    // Static array to store lines of text
    static char lines[MAX_LINES][MAX_STRING_LENGTH];

    // Copy and draw each line of text
    strcpy(lines[0], "    Instructions");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[0], -1, 0, 0, true);
    strcpy(lines[2], "Select the number of");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[2], -1, 0, 16, true);
    strcpy(lines[3], "rounds, players, and");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[3], -1, 0, 24, true);
    strcpy(lines[4], "player's names. Every");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[4], -1, 0, 32, true);
    strcpy(lines[5], "round all players");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[5], -1, 0, 40, true);
    strcpy(lines[6], "enter their choice.");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[6], -1, 0, 48, true);
    strcpy(lines[7], "whoever wins the");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[7], -1, 0, 56, true);
    strcpy(lines[8], "round gets a point.");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[8], -1, 0, 64, true);
    strcpy(lines[9], "After all rounds are");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[9], -1, 0, 72, true);
    strcpy(lines[10], "played, the scores");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[10], -1, 0, 80, true);
    strcpy(lines[11], "and winners are shown.");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[11], -1, 0, 88, true);
    strcpy(lines[13], "LB2: Go Back");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[13], -1, 0, 104, true);
}


// Function to print settings screen
void print_settings(Application* app_p, HAL* hal_p, bool PR, bool rst){
    // Static array to store lines of text
    static char lines[MAX_LINES][MAX_STRING_LENGTH];
    // Static variables to store positions of asterisk and space
    static int astr_y = PLAYERS_POS;
    static int space_y = ROUNDS_POS;

    // Toggle positions based on input parameters
    Toggle(&astr_y, &space_y, &app_p -> players, &app_p -> rounds, PR, rst);

    // Draw "Choose Settings" text on the screen
    strcpy(lines[0], "   Choose Settings");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[0], -1, 0, 0, true);

    // Draw instructions for changing settings
    strcpy(lines[2], "Press JSB to change #");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[2], -1, 0, 16, true);
    strcpy(lines[3], "Press LB2 to switch");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[3], -1, 0, 24, true);
    strcpy(lines[4], "between players");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[4], -1, 0, 32, true);
    strcpy(lines[5], "and # of Rounds");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[5], -1, 0, 40, true);

    // Draw current number of rounds
    strcpy(lines[7], "# of Rounds: ");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[7], -1, 5, 56, true);
    sprintf(lines[8], "%d", app_p -> rounds);
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[8], -1, 90, 56, true);

    // Draw current number of players
    strcpy(lines[9], "# of Players:");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[9], -1, 5, 72, true);
    sprintf(lines[10], "%d", app_p -> players);
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[10], -1, 90, 72, true);

    // Draw confirmation and reset options
    strcpy(lines[11], "BB1: Confirm");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[11], -1, 5, 88, true);
    strcpy(lines[12], "LB1: Reset Settings");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[12], -1, 5, 96, true);

    // Draw asterisk and space indicators
    strcpy(lines[15], "*");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[15], -1, 105, astr_y, true);
    strcpy(lines[14], " ");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[14], -1, 105, space_y, true);
}


// Function to print the name selection screen
void print_selection(Application* app_p, HAL* hal_p){
    // Static array to store lines of text
    static char lines[MAX_LINES][MAX_STRING_LENGTH];
    // Static variables to store positions of asterisk and space
    static int astr_y = 0;
    static int space_y = 8;

    // Update positions based on number of players
    if (astr_y / Y_INCREMENTAL < app_p -> players){
        if (astr_y != 0)
            space_y = astr_y;
        astr_y += Y_INCREMENTAL;
    }

    // Draw "Name Select Screen" text on the screen
    strcpy(lines[0], "Name Select Screen");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[0], -1, 0, 0, true);

    static int i;

    // Draw player numbers for selection
    for (i = 1; i <= app_p->players; i++) {
        sprintf(lines[i], "%d)", i);
        Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[i], -1, 10, i*8, true);
    }

    // Draw instructions for name input
    strcpy(lines[6], "Type 3 letters into");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[6], -1, 0, 40, true);
    strcpy(lines[7], "the UART terminal,");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[7], -1, 0, 48, true);
    strcpy(lines[8], "then press BB1 to");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[8], -1, 0, 56, true);
    strcpy(lines[9], "move to the next");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[9], -1, 0, 64, true);
    strcpy(lines[10], "player or start the");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[10], -1, 0, 72, true);
    strcpy(lines[11], "game.");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[11], -1, 0, 80, true);

    // Draw space and asterisk indicators
    strcpy(lines[14], " ");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[14], -1, 0, space_y, true);
    strcpy(lines[15], "*");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) lines[15], -1, 0, astr_y, true);
}


// Function to print the game screen
void print_game(Application* app_p, HAL* hal_p){
    // Static array to store game text
    static char game_text[] = "Game Screen";
    // Draw the game screen text on the display
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) game_text, -1, 0, 0, true);
}

// Function to print the message for pressing BB1 to play the round
void print_BB1(Application* app_p, HAL* hal_p){
    // Static variables and text for the message
    static int i;
    static char BB1_text[] = "Press BB1 to play the round";
    // Move to a new line in UART output
    uart_new_line(hal_p);
    // Loop through the characters in the text and send them via UART
    for (i = 0; i<strlen(BB1_text); i++)
        UART_sendChar(&hal_p->uart, BB1_text[i]);
}

// Function to print the message for pressing BB1 to end the game
void print_BB1_end(Application* app_p, HAL* hal_p){
    // Static variables and text for the message
    static int i;
    static char BB1_text[] = "\r\nPress BB1 to end the game";
    // Move to a new line in UART output
    uart_new_line(hal_p);
    // Loop through the characters in the text and send them via UART
    for (i = 0; i<strlen(BB1_text); i++)
        UART_sendChar(&hal_p->uart, BB1_text[i]);
}

// Function to print the message for pressing BB1 to end
void print_BB1_end_screen(Application* app_p, HAL* hal_p){
    // Static text for the message
    static char BB1_text[] = "Press BB1 to end";
    // Draw the message on the display
    Graphics_drawString(&hal_p->g_sContext, (int8_t *) BB1_text, -1, 20, 64, true);
}


// Function to print scores
void print_scores(Application* app_p, HAL* hal_p){
    // Static array to store lines
    static char lines[MAX_LINES][MAX_STRING_LENGTH];

    // Static variable to store loop index
    static int i;

    // Loop through players
    for (i = 0; i < app_p->players; i++){
        // Check if the index is even
        if (i % 2 == 0){
            // Copy player name to line array
            strcpy(lines[i], app_p->names[i]);
            // Draw player name on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i], -1, 5, (i + 1) * 24, true);

            // Copy player choice to line array
            strcpy(lines[i + 4], app_p->choices[i]);
            // Draw player choice on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)app_p->choices[i], -1, 5, ((i + 1) * 24) + 8, true);

            // Draw "wins:" on the display
            strcpy(lines[i + 8], "wins:");
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 8], -1, 5, ((i + 1) * 24) + 16, true);

            // Copy player wins to line array
            strcpy(lines[i + 12], "");
            sprintf(lines[i + 12] + strlen(lines[i + 12]), "%d", app_p->wins[i]);
            // Draw player wins on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 12], -1, 35, ((i + 1) * 24) + 16, true);
        } else {
            // Copy player name to line array
            strcpy(lines[i], app_p->names[i]);
            // Draw player name on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i], -1, 85, i * 24, true);

            // Copy player choice to line array
            strcpy(lines[i + 4], app_p->choices[i]);
            // Draw player choice on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 4], -1, 85, (i * 24) + 8, true);

            // Draw "wins:" on the display
            strcpy(lines[8], "wins:");
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[8], -1, 85, (i * 24) + 16, true);

            // Copy player wins to line array
            strcpy(lines[i + 12], "");
            sprintf(lines[i + 12] + strlen(lines[i + 12]), "%d", app_p->wins[i]);
            // Draw player wins on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 12], -1, 115, (i * 24) + 16, true);
        }
    }

    // Draw "Round" on the display
    strcpy(lines[8], "Round");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[8], -1, 45, 56, true);

    // Copy rounds count to line array
    strcpy(lines[9], "");
    sprintf(lines[9] + strlen(lines[i + 12]), "%d", app_p->rounds_count);
    // Draw rounds count on the display
    Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[9], -1, 80, 56, true);
}


// Function to handle invalid input
void invalid_input(HAL* hal_p){
    // Error message
    char error_msg[] = "Enter an R/r/P/p/S/s to choose\r\n";
    // Static variable to store loop index
    static int i;
    // Check if UART can send
    if (UART_canSend(&hal_p->uart)){
        // Loop through the error message and send each character via UART
        for (i = 0; i < strlen(error_msg); i++)
            UART_sendChar(&hal_p->uart, error_msg[i]);
    }
}

// Function to prompt user to enter name and game choices
void uart_name(HAL* hal_p, char* name){
    // Text prompting user to enter name and game choices
    char game_text[] = ", please enter\r\nR or r for Rock\r\nP or p for Paper\r\nS or s for Scissors\r\n";
    // Static variable to store loop index
    static int i;
    // New line in UART
    uart_new_line(hal_p);
    // Loop through the name and send each character via UART
    for (i = 0; i < strlen(name); i++)
        UART_sendChar(&hal_p->uart, name[i]);
    // Loop through the game text and send each character via UART
    for (i = 0; i < strlen(game_text); i++)
        UART_sendChar(&hal_p->uart, game_text[i]);
}

// Function to print the end screen with winners and scores
void print_over(Application* app_p, HAL* hal_p){
    // Static array to store lines
    static char lines[MAX_LINES][MAX_STRING_LENGTH];
    // Draw "End Screen" on the display
    strcpy(lines[13], "End Screen");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[13], -1, 0, 0, true);
    // Static variables to store loop indexes and winner count
    static int i, j = 1;
    // Loop through players
    for (i = 0; i < app_p->players; i++){
        // Copy player name to line array
        strcpy(lines[i], app_p->names[i]);
        // Draw player name on the display
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i], -1, 0, (24 * i) + 16, true);
        // Draw "wins:" on the display
        strcpy(lines[i + 8], "wins:");
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 8], -1, 0, ((24 * i) + 16) + 8, true);
        // Copy player wins to line array
        strcpy(lines[i + 4], "");
        sprintf(lines[i + 4] + strlen(lines[i + 4]), "%d", app_p->wins[i]);
        // Draw player wins on the display
        Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[i + 4], -1, 30, ((24 * i) + 16) + 8, true);
    }
    // Draw "Winners:" on the display
    strcpy(lines[12], "Winners:");
    Graphics_drawString(&hal_p->g_sContext, (int8_t *)lines[12], -1, 50, 32, true);
    // Find the maximum wins
    int max = app_p->wins[0];
    for (i = 1; i < app_p->players; i++){
        if (app_p->wins[i] > max)
            max = app_p->wins[i];
    }
    // Loop through players to find winners
    for (i = 0; i < app_p->players; i++){
        if (app_p->wins[i] == max){
            // Draw winner names on the display
            Graphics_drawString(&hal_p->g_sContext, (int8_t *)app_p->names[i], -1, 50, (8 * j) + 32, true);
            j++;
        }
    }
}

// Function to clear the screen
void clear_screen(HAL* hal_p){
    // Define the screen rectangle
    static Graphics_Rectangle R;
    R.xMin = 0;
    R.xMax = 127;
    R.yMin = 0;
    R.yMax = 127;

    // Set foreground color to black and fill the rectangle
    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_BLACK);
    Graphics_fillRectangle(&hal_p->g_sContext, &R);
    // Set foreground color back to white
    Graphics_setForegroundColor(&hal_p->g_sContext, GRAPHICS_COLOR_WHITE);
}

// Function to toggle between players and rounds or reset settings
void Toggle(int* astr_y, int* space_y, int* players, int* rounds, bool PR, bool rst){

    if (PR && !rst){
        PR_Toggle(astr_y);
        PR_Toggle(space_y);
    }
    else if (!PR && (*astr_y == ROUNDS_POS) && !rst){
        RoundIncrement(rounds);
    }
    else if (!PR && (*astr_y == PLAYERS_POS) && !rst){
        PlayerIncrement(players);
    }
    else if (!PR && rst){
        *players = DEF_PLAYERS;
        *rounds = DEF_ROUNDS;
    }

}

// Function to toggle between players and rounds positions
void PR_Toggle(int *currentNum) {
    if (*currentNum == ROUNDS_POS)
        *currentNum = PLAYERS_POS;
    else if (*currentNum == PLAYERS_POS)
        *currentNum = ROUNDS_POS;
}

// Function to increment the number of players
void PlayerIncrement(int *currentNum) {
    if (*currentNum + 1 == MAX_PLAYERS)
        *currentNum = MIN_PLAYERS;
    else
        *currentNum = (*currentNum + 1) % MAX_PLAYERS;
}

// Function to increment the number of rounds
void RoundIncrement(int *currentNum) {
    if (*currentNum + 1 == MAX_ROUNDS)
        *currentNum = MIN_ROUNDS;
    else
        *currentNum = (*currentNum + 1) % MAX_ROUNDS;
}

// Function to determine the winners of the game
void determine_winners(Application* app_p) {
    static int i;
    int r = 0, p = 0, s = 0;
    // Count the number of choices for each option
    for(i = 0; i < app_p -> players; i++) {
        if(app_p -> choices[i][0] == 'r' || app_p -> choices[i][0] == 'R') r++;
        else if(app_p -> choices[i][0] == 'p' || app_p -> choices[i][0] == 'P') p++;
        else if(app_p -> choices[i][0] == 's' || app_p -> choices[i][0] == 'S') s++;
    }

    // If all options are present, no winner
    if(r > 0 && p > 0 && s > 0) {
        return;
    }
    // If all players choose the same option, award each one point
    else if(r == app_p -> players || p == app_p -> players || s == app_p -> players) {
        for(i = 0; i < app_p -> players; i++) {
            app_p -> wins[i]++;
        }
    }
    // Otherwise, determine the winner(s) based on the choices
    else {
        if(r > 0 && s > 0) {
            // Rock beats scissors
            for(i = 0; i < app_p -> players; i++) {
                if(app_p -> choices[i][0] == 'r' || app_p -> choices[i][0] == 'R') app_p -> wins[i]++;
            }
        } else if(s > 0 && p > 0) {
            // Scissors beat paper
            for(i = 0; i < app_p -> players; i++) {
                if(app_p -> choices[i][0] == 's' || app_p -> choices[i][0] == 'S') app_p -> wins[i]++;
            }
        } else if(p > 0 && r > 0) {
            // Paper beats rock
            for(i = 0; i < app_p -> players; i++) {
                if(app_p -> choices[i][0] == 'p' || app_p -> choices[i][0] == 'P') app_p -> wins[i]++;
            }
        }
    }
}

// Function to reset the wins of all players to zero
void wins_rst(Application* app_p){
    static int i;
    for (i = 0; i < app_p -> players; i++)
        app_p -> wins[i] = 0;
}
