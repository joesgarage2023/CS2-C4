#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// Constants
const int RED_PIN = 4;
const int GREEN_PIN = 3;
const int BLUE_PIN = 2;
const int BUZZER_PIN = 5;
const int BOMB_SECONDS_EXPLOSION = 43;
const int BUZZER_FREQUENCY = 4096;
const float INITIAL_BEEP_INTERVAL = 700;
const float FINAL_BEEP_INTERVAL = 50;
const byte ROWS = 4;
const byte COLS = 3;
const char CORRECT_PASSWORD[] = "7355608";
const int MAX_PASSWORD_LENGTH = 7;
const int ANIMATION_INTERVAL = 200;
const int ANIMATION_MIN_COL = 3;
const int ANIMATION_MAX_COL = 10;
const int GREEN_BLINK_INTERVAL = 500;

// Keypad setup
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {10, 11, 12};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// LCD setup
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Variables
char password_store[MAX_PASSWORD_LENGTH + 1] = "";
int pass_stored = 0;
int bomb_status = 0;
int bomb_steps = 0;
unsigned long bomb_secs_prev = 0;
unsigned long buzzerled_prev = 0;
unsigned long animation_prev = 0;
unsigned long green_blink_prev = 0;
float buzzerled_step = INITIAL_BEEP_INTERVAL;
int led_state = LOW;
int green_led_state = LOW;
int show_info = 0;
int animation_col = 4;
int animation_dir = 1;
int last_animation_col = 4;

void setup() {
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  lcd.begin(16, 2);
  lcd.backlight();
  tone(BUZZER_PIN, BUZZER_FREQUENCY, 100);
  delay(200);
  tone(BUZZER_PIN, BUZZER_FREQUENCY, 100);
  delay(200);
  lcd.setCursor(4, 0);
  lcd.print("*******");
}

void clearScreen() {
  lcd.clear();
}

void bombBlew() {
  tone(BUZZER_PIN, 0, 100);
  led_state = LOW;
  buzzerled_step = INITIAL_BEEP_INTERVAL;
  digitalWrite(RED_PIN, HIGH);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  clearScreen();
  lcd.setCursor(4, 0);
  lcd.print("BOOOOM!");

  // Wait for reset sequence: #123456
  const char reset_code[] = "#123456";
  char input_buffer[8] = "";  // 7 chars + null terminator
  int input_index = 0;

  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (input_index < 7) {
        input_buffer[input_index++] = key;
        input_buffer[input_index] = '\0';
      }

      // Show typed input on LCD line 2
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(input_buffer);

      if (strcmp(input_buffer, reset_code) == 0) {
        break;
      }

      // Reset buffer on incorrect full entry
      if (input_index >= 7 && strcmp(input_buffer, reset_code) != 0) {
        input_index = 0;
        input_buffer[0] = '\0';
      }
    }
  }
  adBomb(false); // Reset game
}

void adBomb(bool activate) {
  password_store[0] = '\0';
  pass_stored = 0;
  bomb_steps = 0;
  bomb_secs_prev = 0;
  buzzerled_prev = 0;
  buzzerled_step = INITIAL_BEEP_INTERVAL;
  animation_col = 4;
  last_animation_col = 4;
  animation_dir = 1;
  digitalWrite(RED_PIN, LOW);
  digitalWrite(GREEN_PIN, LOW);
  digitalWrite(BLUE_PIN, LOW);
  green_led_state = LOW;
  clearScreen();
  if (activate) {
    bomb_status = 1;
    show_info = 0;
    lcd.setCursor(animation_col, 0);
    lcd.print('*');
  } else {
    lcd.setCursor(4, 0);
    lcd.print("*******");
    bomb_status = 0;
  }
}

void bombAdjust() {
  if (led_state == LOW) {
    led_state = HIGH;
    tone(BUZZER_PIN, BUZZER_FREQUENCY, 100);
    digitalWrite(RED_PIN, HIGH);
  } else {
    led_state = LOW;
    tone(BUZZER_PIN, 0, 100);
    digitalWrite(RED_PIN, LOW);
  }
}

void displayPassword() {
  clearScreen();
  lcd.setCursor(4, 0);
  for (int i = 0; i < 7; i++) {
    if (i < 7 - pass_stored) {
      lcd.print('*');
    } else {
      lcd.print(password_store[pass_stored - (7 - i)]);
    }
  }
}

void displayAnimation() {
  lcd.setCursor(last_animation_col, 0);
  lcd.print(' ');
  lcd.setCursor(animation_col, 0);
  lcd.print('*');
  last_animation_col = animation_col;
}

void playDefuseSound() {
  for (int freq = 1000; freq <= 4000; freq += 200) {
    tone(BUZZER_PIN, freq);
    delay(50);
  }
  noTone(BUZZER_PIN);
}

void loop() {
  char key = keypad.getKey();
  unsigned long current_millis = millis();

  if (bomb_status == 0) {
    if (current_millis - green_blink_prev >= GREEN_BLINK_INTERVAL) {
      green_led_state = !green_led_state;
      digitalWrite(GREEN_PIN, green_led_state);
      green_blink_prev = current_millis;
    }
  }

  if (key) {
    if (key == '#') {
      adBomb(bomb_status);
      return;
    } else {
      if (pass_stored < MAX_PASSWORD_LENGTH) {
        password_store[pass_stored++] = key;
        password_store[pass_stored] = '\0';
        displayPassword();
      }
    }
  }

  if (pass_stored == MAX_PASSWORD_LENGTH) {
    if (strcmp(password_store, CORRECT_PASSWORD) == 0) {
      if (bomb_status == 0) {
        digitalWrite(GREEN_PIN, LOW);
        green_led_state = LOW;
        adBomb(true);
      } else {
        adBomb(false);
        playDefuseSound();
        clearScreen();
        lcd.print("DEFUSED");
        digitalWrite(GREEN_PIN, HIGH);
        delay(1500);
        digitalWrite(GREEN_PIN, LOW);
        green_led_state = LOW;
        clearScreen();
        lcd.setCursor(4, 0);
        lcd.print("*******");
      }
    } else {
      clearScreen();
      lcd.print("WRONG PASSWORD");
      digitalWrite(BLUE_PIN, HIGH);
      delay(1000);
      digitalWrite(BLUE_PIN, LOW);
      adBomb(bomb_status);
    }
    return;
  }

  if (bomb_status == 1) {
    if (current_millis - animation_prev >= ANIMATION_INTERVAL) {
      animation_col += animation_dir;
      if (animation_col >= ANIMATION_MAX_COL || animation_col <= ANIMATION_MIN_COL) {
        animation_dir = -animation_dir;
      }
      animation_prev = current_millis;
      displayAnimation();
    }

    if (current_millis - buzzerled_prev >= buzzerled_step) {
      buzzerled_prev = current_millis;
      bombAdjust();
      float remaining_secs = BOMB_SECONDS_EXPLOSION - bomb_steps;
      if (remaining_secs > 0) {
        buzzerled_step = FINAL_BEEP_INTERVAL + (INITIAL_BEEP_INTERVAL - FINAL_BEEP_INTERVAL) * (remaining_secs / BOMB_SECONDS_EXPLOSION);
      } else {
        buzzerled_step = FINAL_BEEP_INTERVAL;
      }
    }

    if (current_millis - bomb_secs_prev >= 1000) {
      bomb_secs_prev = current_millis;
      bomb_steps++;
      if (bomb_steps >= BOMB_SECONDS_EXPLOSION) {
        bombBlew();
      }
    }
  }

  if (bomb_status == 0 && show_info == 0) {
    clearScreen();
    lcd.setCursor(4, 0);
    lcd.print("*******");
    show_info = 1;
  }
}