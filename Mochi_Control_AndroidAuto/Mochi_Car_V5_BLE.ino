#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <math.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <cstring>

// =====================================================
// MOCHI CAR V5 BLE - BASE V4 + EXPRESSOES V3.3 + CONTROLE ANDROID AUTO
// ESP32 DevKit V1 | OLED SSD1306 128x64
//
// OLED: SDA GPIO21 | SCL GPIO22 | endereco 0x3C
// BOTAO MENU: GPIO18 -> GND
// BOTAO ANIMACOES: GPIO19 -> GND
//
// Ambos os botoes usam INPUT_PULLUP.
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

#define SDA_PIN 21
#define SCL_PIN 22
#define BUTTON_MENU_PIN 18
#define BUTTON_ANIM_PIN 19

#define DEBOUNCE_MS 40
#define LONG_PRESS_MS 1000
#define TRIP_RESET_HOLD_MS 1500

#define MUSIC_FRAME_MS 45
#define TRIP_FRAME_MS 200

// BLE: o celular conecta ao Mochi por Bluetooth Low Energy.
// O Wi-Fi do celular permanece livre para o Android Auto sem fio.
static const char* BLE_DEVICE_NAME = "Mochi-Car";
static const char* BLE_SERVICE_UUID = "8b6f0001-6f3a-4b9d-9a4e-2a1b7c8d0001";
static const char* BLE_COMMAND_UUID = "8b6f0002-6f3a-4b9d-9a4e-2a1b7c8d0001";
BLEServer* bleServer = nullptr;
BLECharacteristic* bleCommandCharacteristic = nullptr;
volatile bool bleClientConnected = false;
unsigned long lastBleStatus = 0;
portMUX_TYPE bleMux = portMUX_INITIALIZER_UNLOCKED;
char pendingBleCommand[40] = {0};
volatile bool bleCommandPending = false;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RoboEyes<Adafruit_SSD1306> roboEyes(display);

// ---------------- PERSONALIDADES DA V3.3 ----------------

enum MochiMode {
  MODE_NORMAL,
  MODE_CUTE,
  MODE_FUNNY,
  MODE_GRUMPY,
  MODE_SLEEP,
  MODE_AUTOMATIC
};

MochiMode currentMode = MODE_NORMAL;

const char* modeNames[] = {
  "NORMAL", "FOFO", "ENGRACADO",
  "RABUGENTO", "SONO", "AUTOMATICO"
};

// ---------------- AS 11 EXPRESSOES DA V3.3 ----------------

enum MochiScene {
  SCENE_NORMAL,
  SCENE_HAPPY,
  SCENE_SLEEPY,
  SCENE_GRUMPY,
  SCENE_CURIOUS,
  SCENE_EXCITED,
  SCENE_SHY,
  SCENE_SURPRISED,
  SCENE_CONFUSED,
  SCENE_LAUGHING,
  SCENE_BORED,
  SCENE_DEBOCHADO,
  SCENE_GARGALHADA,
  SCENE_PISCADINHA,
  SCENE_CONFIANTE,
  SCENE_DUVIDANDO,
  SCENE_IMPRESSIONADO,
  SCENE_IRRITADO,
  SCENE_SIMPATICO,
  SCENE_OLA,
  SCENE_OBRIGADO,
  SCENE_FOI_MAL,
  SCENE_CARINHO
};

MochiScene currentScene = SCENE_NORMAL;

const char* sceneNames[] = {
  "Normal", "Feliz", "Sonolento", "Rabugento",
  "Curioso", "Empolgado", "Timido", "Surpreso",
  "Confuso", "Rindo", "Entediado",
  "Debochado", "Gargalhada", "Piscadinha", "Confiante",
  "Duvidando", "Impressionado", "Irritado", "Simpatico",
  "Ola", "Obrigado", "Foi mal", "Carinho"
};

// ---------------- TELAS ----------------

enum ScreenState {
  SCREEN_HOME,
  SCREEN_MENU,
  SCREEN_MUSIC,
  SCREEN_ANIMATIONS,
  SCREEN_TRIP
};

ScreenState currentScreen = SCREEN_HOME;

// Prototipos usados antes das implementacoes.
const char* getModeName();
unsigned long getTripTime();
void setScreenRemote(ScreenState target);

// ---------------- MENU ----------------

const char* menuItems[] = {
  "Music",
  "Animations",
  "Trip",
  "Voltar"
};

const uint8_t MENU_COUNT = 4;
uint8_t menuSelection = 0;
bool menuNeedsDraw = true;

// ---------------- BOTOES COM DEBOUNCE ----------------

struct Button {
  uint8_t pin;
  bool rawState;
  bool stableState;
  unsigned long lastChange;
  unsigned long pressedAt;
  bool longHandled;
};

Button menuButton = {
  BUTTON_MENU_PIN, HIGH, HIGH, 0, 0, false
};

Button animButton = {
  BUTTON_ANIM_PIN, HIGH, HIGH, 0, 0, false
};

// ---------------- TEMPOS E EVENTOS DA V3.3 ----------------

const unsigned long MIN_SCENE_TIME = 5500;
const unsigned long MAX_SCENE_TIME = 10000;

unsigned long nextSceneTime = 0;
unsigned long nextLookTime = 0;
unsigned long nextSpecialTime = 0;
unsigned long specialUntil = 0;

bool specialActive = false;
bool lookingRight = true;

bool startupSequence = true;
unsigned long startupTime = 0;
uint8_t startupStage = 0;

bool transitionPending = false;
unsigned long transitionUntil = 0;

// ---------------- MUSIC ----------------

uint8_t musicStyle = 0;
const char* musicStyleNames[] = {
  "WAVE", "PULSE", "RANDOM"
};

// ---------------- TRIP ----------------
// Estas variaveis ficam globais e NAO sao reiniciadas ao trocar de tela.
// Assim, o cronometro continua contando em segundo plano quando o usuario
// sai de Trip e abre Menu, Music, Animations ou Home.

bool tripRunning = false;
unsigned long tripStartMillis = 0;
unsigned long tripElapsedMillis = 0;

// ---------------- ATUALIZACAO DO DISPLAY ----------------

unsigned long lastMusicFrame = 0;
unsigned long lastTripFrame = 0;
bool screenNeedsRedraw = true;


// =====================================================
// BLE - RECEPCAO DE COMANDOS DO MOCHI CONTROL
// =====================================================

class MochiBleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    bleClientConnected = true;
    Serial.println("Celular conectado ao Mochi por BLE.");
  }

  void onDisconnect(BLEServer* server) override {
    bleClientConnected = false;
    Serial.println("Celular desconectado do Mochi.");
    BLEDevice::startAdvertising();
  }
};

class MochiBleCommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    // Nas versoes atuais da biblioteca BLE do ESP32, getValue() retorna String.
    String value = characteristic->getValue();
    if (value.length() == 0) return;

    portENTER_CRITICAL(&bleMux);
    size_t n = value.length();
    if (n >= sizeof(pendingBleCommand)) n = sizeof(pendingBleCommand) - 1;
    memcpy(pendingBleCommand, value.c_str(), n);
    pendingBleCommand[n] = '\0';
    bleCommandPending = true;
    portEXIT_CRITICAL(&bleMux);
  }
};

void startBleControl() {
  BLEDevice::init(BLE_DEVICE_NAME);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new MochiBleServerCallbacks());

  BLEService* service = bleServer->createService(BLE_SERVICE_UUID);

  bleCommandCharacteristic = service->createCharacteristic(
    BLE_COMMAND_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  bleCommandCharacteristic->addDescriptor(new BLE2902());
  bleCommandCharacteristic->setCallbacks(new MochiBleCommandCallbacks());
  bleCommandCharacteristic->setValue("MOCHI_READY");

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(BLE_SERVICE_UUID);
  advertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE Mochi iniciado. Nome: Mochi-Car");
}

void sendBleStatus() {
  if (!bleClientConnected || bleCommandCharacteristic == nullptr) return;
  if ((unsigned long)(millis() - lastBleStatus) < 1000) return;
  lastBleStatus = millis();

  char status[180];
  snprintf(status, sizeof(status),
    "MODE=%s;SCENE=%u;TRIP_MS=%lu;TRIP_RUNNING=%u;MUSIC=%u;SCREEN=%u",
    getModeName(),
    (unsigned int)currentScene,
    getTripTime(),
    tripRunning ? 1 : 0,
    (unsigned int)musicStyle,
    (unsigned int)currentScreen
  );

  bleCommandCharacteristic->setValue((uint8_t*)status, strlen(status));
  bleCommandCharacteristic->notify();
}

// =====================================================
// UTILITARIOS
// =====================================================

unsigned long randomBetween(unsigned long minimum, unsigned long maximum) {
  return random(minimum, maximum + 1);
}

void scheduleNextScene() {
  nextSceneTime = millis() + randomBetween(MIN_SCENE_TIME, MAX_SCENE_TIME);
}

void scheduleNextSpecial() {
  nextSpecialTime = millis() + randomBetween(35000, 60000);
}

void normalEyes() {
  roboEyes.setWidth(38, 38);
  roboEyes.setHeight(38, 38);
  roboEyes.setBorderradius(12, 12);
  roboEyes.setSpacebetween(12);
}

const char* getModeName() {
  return modeNames[(uint8_t)currentMode];
}

// =====================================================
// PERSONALIDADES
// =====================================================

void applyMode() {
  roboEyes.setPosition(DEFAULT);
  roboEyes.setCuriosity(OFF);

  switch (currentMode) {
    case MODE_NORMAL:
      roboEyes.setMood(DEFAULT);
      roboEyes.setAutoblinker(ON, 3, 2);
      roboEyes.setIdleMode(ON, 2, 2);
      break;

    case MODE_CUTE:
      roboEyes.setMood(HAPPY);
      roboEyes.setCuriosity(ON);
      roboEyes.setAutoblinker(ON, 2, 2);
      roboEyes.setIdleMode(ON, 2, 2);
      break;

    case MODE_FUNNY:
      roboEyes.setMood(HAPPY);
      roboEyes.setCuriosity(ON);
      roboEyes.setAutoblinker(ON, 2, 1);
      roboEyes.setIdleMode(ON, 1, 2);
      break;

    case MODE_GRUMPY:
      roboEyes.setMood(ANGRY);
      roboEyes.setAutoblinker(ON, 4, 2);
      roboEyes.setIdleMode(ON, 3, 2);
      break;

    case MODE_SLEEP:
      roboEyes.setMood(TIRED);
      roboEyes.setAutoblinker(ON, 5, 3);
      roboEyes.setIdleMode(OFF, 2, 1);
      break;

    case MODE_AUTOMATIC:
      roboEyes.setMood(DEFAULT);
      roboEyes.setAutoblinker(ON, 2, 2);
      roboEyes.setIdleMode(ON, 2, 2);
      break;
  }
}

// =====================================================
// ESCOLHA DE EXPRESSOES - COPIADA DA LOGICA V3.3
// =====================================================

MochiScene chooseSceneForMode() {
  static const MochiScene normalPool[] = {
    SCENE_NORMAL, SCENE_NORMAL, SCENE_HAPPY, SCENE_HAPPY,
    SCENE_CURIOUS, SCENE_CURIOUS, SCENE_SLEEPY, SCENE_GRUMPY,
    SCENE_EXCITED, SCENE_SHY, SCENE_SURPRISED,
    SCENE_CONFUSED, SCENE_LAUGHING, SCENE_BORED
  };

  static const MochiScene cutePool[] = {
    SCENE_NORMAL, SCENE_HAPPY, SCENE_HAPPY, SCENE_SHY,
    SCENE_CURIOUS, SCENE_EXCITED, SCENE_SURPRISED, SCENE_SLEEPY
  };

  static const MochiScene funnyPool[] = {
    SCENE_HAPPY, SCENE_LAUGHING, SCENE_LAUGHING,
    SCENE_CONFUSED, SCENE_SURPRISED, SCENE_EXCITED,
    SCENE_CURIOUS, SCENE_BORED
  };

  static const MochiScene grumpyPool[] = {
    SCENE_GRUMPY, SCENE_GRUMPY, SCENE_BORED,
    SCENE_SLEEPY, SCENE_CONFUSED, SCENE_NORMAL
  };

  static const MochiScene sleepPool[] = {
    SCENE_SLEEPY, SCENE_SLEEPY, SCENE_BORED,
    SCENE_SHY, SCENE_NORMAL
  };

  static const MochiScene automaticPool[] = {
    SCENE_NORMAL, SCENE_HAPPY, SCENE_SLEEPY, SCENE_GRUMPY,
    SCENE_CURIOUS, SCENE_EXCITED, SCENE_SHY, SCENE_SURPRISED,
    SCENE_CONFUSED, SCENE_LAUGHING, SCENE_BORED
  };

  const MochiScene* pool = automaticPool;
  uint8_t count = sizeof(automaticPool) / sizeof(automaticPool[0]);

  switch (currentMode) {
    case MODE_NORMAL:
      pool = normalPool;
      count = sizeof(normalPool) / sizeof(normalPool[0]);
      break;
    case MODE_CUTE:
      pool = cutePool;
      count = sizeof(cutePool) / sizeof(cutePool[0]);
      break;
    case MODE_FUNNY:
      pool = funnyPool;
      count = sizeof(funnyPool) / sizeof(funnyPool[0]);
      break;
    case MODE_GRUMPY:
      pool = grumpyPool;
      count = sizeof(grumpyPool) / sizeof(grumpyPool[0]);
      break;
    case MODE_SLEEP:
      pool = sleepPool;
      count = sizeof(sleepPool) / sizeof(sleepPool[0]);
      break;
    case MODE_AUTOMATIC:
      break;
  }

  MochiScene selected;
  do {
    selected = pool[random(0, count)];
  } while (selected == currentScene && count > 1);

  return selected;
}

// =====================================================
// EXPRESSOES - TODAS AS 11 CENAS DA V3.3
// =====================================================

void startScene(MochiScene scene) {
  currentScene = scene;
  specialActive = false;

  normalEyes();
  roboEyes.setPosition(DEFAULT);
  roboEyes.setMood(DEFAULT);

  switch (scene) {
    case SCENE_NORMAL:
      roboEyes.setMood(DEFAULT);
      break;

    case SCENE_HAPPY:
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      break;

    case SCENE_SLEEPY:
      roboEyes.setMood(TIRED);
      roboEyes.setPosition(S);
      break;

    case SCENE_GRUMPY:
      roboEyes.setMood(ANGRY);
      break;

    case SCENE_CURIOUS:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(E);
      lookingRight = true;
      nextLookTime = millis() + 900;
      break;

    case SCENE_EXCITED:
      roboEyes.setMood(HAPPY);
      roboEyes.setWidth(42, 42);
      roboEyes.setHeight(42, 42);
      roboEyes.setBorderradius(17, 17);
      roboEyes.anim_laugh();
      break;

    case SCENE_SHY:
      roboEyes.setMood(TIRED);
      roboEyes.setPosition(S);
      roboEyes.blink(1, 0);
      break;

    case SCENE_SURPRISED:
      roboEyes.setMood(DEFAULT);
      roboEyes.setWidth(44, 44);
      roboEyes.setHeight(44, 44);
      roboEyes.setBorderradius(20, 20);
      break;

    case SCENE_CONFUSED:
      roboEyes.setMood(DEFAULT);
      roboEyes.anim_confused();
      break;

    case SCENE_LAUGHING:
      roboEyes.setMood(HAPPY);
      roboEyes.anim_laugh();
      nextLookTime = millis() + 2500;
      break;

    case SCENE_BORED:
      roboEyes.setMood(TIRED);
      roboEyes.setPosition(W);
      lookingRight = false;
      nextLookTime = millis() + 1500;
      break;

    // Reacoes personalizadas: usam os mesmos olhos RoboEyes originais.
    case SCENE_DEBOCHADO:
      roboEyes.setMood(HAPPY);
      roboEyes.setPosition(E);
      roboEyes.setWidth(40, 34);
      roboEyes.anim_laugh();
      break;
    case SCENE_GARGALHADA:
      roboEyes.setMood(HAPPY);
      roboEyes.setWidth(44, 44);
      roboEyes.anim_laugh();
      break;
    case SCENE_PISCADINHA:
      roboEyes.setMood(HAPPY);
      roboEyes.setPosition(DEFAULT);
      roboEyes.blink(1, 0);
      break;
    case SCENE_CONFIANTE:
      roboEyes.setMood(ANGRY);
      roboEyes.setPosition(DEFAULT);
      roboEyes.setHeight(30, 30);
      break;
    case SCENE_DUVIDANDO:
      roboEyes.setMood(DEFAULT);
      roboEyes.setPosition(E);
      roboEyes.anim_confused();
      break;
    case SCENE_IMPRESSIONADO:
      roboEyes.setMood(DEFAULT);
      roboEyes.setWidth(46, 46);
      roboEyes.setHeight(46, 46);
      roboEyes.setBorderradius(22, 22);
      break;
    case SCENE_IRRITADO:
      roboEyes.setMood(ANGRY);
      roboEyes.setPosition(S);
      break;
    case SCENE_SIMPATICO:
      roboEyes.setMood(HAPPY);
      roboEyes.setPosition(DEFAULT);
      break;
    case SCENE_OLA:
      roboEyes.setMood(HAPPY);
      roboEyes.setPosition(W);
      roboEyes.blink(1, 0);
      nextLookTime = millis() + 500;
      break;
    case SCENE_OBRIGADO:
      roboEyes.setMood(HAPPY);
      roboEyes.setPosition(S);
      roboEyes.blink(1, 0);
      break;
    case SCENE_FOI_MAL:
      roboEyes.setMood(TIRED);
      roboEyes.setPosition(S);
      roboEyes.blink(1, 0);
      break;
    case SCENE_CARINHO:
      roboEyes.setMood(HAPPY);
      roboEyes.setWidth(42, 42);
      roboEyes.setHeight(34, 34);
      roboEyes.setBorderradius(17, 17);
      break;
  }

  scheduleNextScene();

  Serial.print("Expressao: ");
  Serial.println(sceneNames[(uint8_t)scene]);
}

// =====================================================
// TRANSICAO DE PERSONALIDADE
// =====================================================

void changeMode() {
  if (transitionPending) return;

  currentMode = (MochiMode)(((uint8_t)currentMode + 1) % 6);

  specialActive = false;
  roboEyes.close();

  transitionPending = true;
  transitionUntil = millis() + 250;

  Serial.print("Nova personalidade: ");
  Serial.println(getModeName());
}

void updateTransition() {
  if (!transitionPending) return;

  if ((long)(millis() - transitionUntil) >= 0) {
    transitionPending = false;
    applyMode();
    startScene(chooseSceneForMode());
    scheduleNextSpecial();
  }
}

// =====================================================
// EVENTOS ESPONTANEOS DA V3.3
// =====================================================

void startSpecialEvent() {
  if (specialActive) return;

  uint8_t event = random(0, 4);

  specialActive = true;
  specialUntil = millis() + 1300;

  switch (event) {
    case 0:
      roboEyes.blink(1, 0);
      Serial.println("Evento: piscadela");
      break;

    case 1:
      roboEyes.anim_laugh();
      Serial.println("Evento: risada");
      break;

    case 2:
      roboEyes.anim_confused();
      Serial.println("Evento: confusao");
      break;

    case 3:
      roboEyes.setMood(TIRED);
      roboEyes.close();
      Serial.println("Evento: cochilo");
      break;
  }
}

void updateSpecialEvent() {
  if (!specialActive) return;

  if ((long)(millis() - specialUntil) >= 0) {
    specialActive = false;
    applyMode();
    startScene(currentScene);
  }
}

void updateSceneMotion() {
  if (specialActive || transitionPending) return;

  if (currentScene == SCENE_CURIOUS || currentScene == SCENE_BORED) {
    if ((long)(millis() - nextLookTime) >= 0) {
      roboEyes.setPosition(lookingRight ? W : E);
      lookingRight = !lookingRight;
      nextLookTime = millis() + randomBetween(700, 1800);
    }
  }

  if (currentScene == SCENE_LAUGHING &&
      (long)(millis() - nextLookTime) >= 0) {
    roboEyes.anim_laugh();
    nextLookTime = millis() + 2500;
  }
}

void updateHomeAnimations() {
  if (startupSequence) {
    unsigned long elapsed = millis() - startupTime;

    if (elapsed < 700) {
      roboEyes.close();
    } else if (elapsed < 1600) {
      roboEyes.open();
      roboEyes.setMood(DEFAULT);
    } else if (elapsed < 2600) {
      roboEyes.setMood(HAPPY);
    } else {
      startupSequence = false;
      applyMode();
      startScene(SCENE_NORMAL);
      scheduleNextSpecial();
    }
    return;
  }

  updateTransition();
  if (transitionPending) return;

  updateSpecialEvent();
  if (specialActive) return;

  updateSceneMotion();

  if ((long)(millis() - nextSceneTime) >= 0) {
    startScene(chooseSceneForMode());
  }

  if ((long)(millis() - nextSpecialTime) >= 0) {
    startSpecialEvent();
    scheduleNextSpecial();
  }
}

// =====================================================
// MENU / TELAS
// =====================================================

void drawMenu() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(38, 2);
  display.print("MOCHI MENU");
  display.drawLine(0, 12, 127, 12, WHITE);

  for (uint8_t i = 0; i < MENU_COUNT; i++) {
    int y = 17 + i * 11;

    if (i == menuSelection) {
      display.fillRect(4, y - 1, 120, 10, WHITE);
      display.setTextColor(BLACK);
      display.setCursor(10, y);
      display.print("> ");
      display.print(menuItems[i]);
      display.setTextColor(WHITE);
    } else {
      display.setCursor(10, y);
      display.print("  ");
      display.print(menuItems[i]);
    }
  }

  display.setCursor(2, 57);
  display.print("MENU: prox. ANIM: entrar");
  display.display();
}

void drawMusic() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(2, 2);
  display.print("MUSIC - ");
  display.print(musicStyleNames[musicStyle]);
  display.drawLine(0, 13, 127, 13, WHITE);

  const int barCount = 16;
  const int barWidth = 5;
  const int gap = 3;
  unsigned long t = millis();

  for (int i = 0; i < barCount; i++) {
    float value;

    if (musicStyle == 0) {
      value = fabs(sin((t / 180.0) + i * 0.55));
    } else if (musicStyle == 1) {
      value = fabs(sin(t / 250.0)) *
              (0.30 + 0.70 * fabs(sin(i * 0.7)));
    } else {
      value = random(25, 100) / 100.0;
    }

    int h = 4 + (int)(value * 39);
    int x = 2 + i * (barWidth + gap);
    display.fillRect(x, 60 - h, barWidth, h, WHITE);
  }

  display.display();
}

unsigned long getTripTime() {
  if (tripRunning) {
    return tripElapsedMillis + (millis() - tripStartMillis);
  }
  return tripElapsedMillis;
}

void formatTripTime(char* buffer, size_t bufferSize) {
  unsigned long totalSeconds = getTripTime() / 1000;
  unsigned int hours = totalSeconds / 3600;
  unsigned int minutes = (totalSeconds % 3600) / 60;
  unsigned int seconds = totalSeconds % 60;

  snprintf(buffer, bufferSize, "%02u:%02u:%02u",
           hours, minutes, seconds);
}

void drawTrip() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);

  display.setCursor(39, 5);
  display.print("TRIP TIMER");
  display.drawLine(0, 17, 127, 17, WHITE);

  char timeBuffer[16];
  formatTripTime(timeBuffer, sizeof(timeBuffer));

  display.setTextSize(2);
  display.setCursor(16, 25);
  display.print(timeBuffer);

  display.setTextSize(1);
  display.setCursor(25, 52);
  display.print(tripRunning ? "EM ANDAMENTO" : "PAUSADO");
  display.display();
}

void enterMenu() {
  currentScreen = SCREEN_MENU;
  menuSelection = 0;
  menuNeedsDraw = true;
  screenNeedsRedraw = true;
}

void returnHome() {
  currentScreen = SCREEN_HOME;
  specialActive = false;
  transitionPending = false;
  applyMode();
  startScene(chooseSceneForMode());
  scheduleNextSpecial();
}

void selectMenuItem() {
  switch (menuSelection) {
    case 0:
      currentScreen = SCREEN_MUSIC;
      break;

    case 1:
      currentScreen = SCREEN_ANIMATIONS;
      applyMode();
      startScene(chooseSceneForMode());
      break;

    case 2:
      currentScreen = SCREEN_TRIP;
      break;

    case 3:
      returnHome();
      break;
  }

  screenNeedsRedraw = true;
}

// =====================================================
// CRONOMETRO: INICIAR / PAUSAR / ZERAR
// =====================================================

void toggleTrip() {
  if (tripRunning) {
    tripElapsedMillis += millis() - tripStartMillis;
    tripRunning = false;
  } else {
    tripStartMillis = millis();
    tripRunning = true;
  }
}

void resetTrip() {
  tripElapsedMillis = 0;
  if (tripRunning) {
    tripStartMillis = millis();
  }
  screenNeedsRedraw = true;
  Serial.println("Trip zerado");
}


void processPendingBleCommand() {
  char command[sizeof(pendingBleCommand)];
  bool hasCommand = false;

  portENTER_CRITICAL(&bleMux);
  if (bleCommandPending) {
    strncpy(command, pendingBleCommand, sizeof(command));
    command[sizeof(command) - 1] = '\0';
    bleCommandPending = false;
    hasCommand = true;
  }
  portEXIT_CRITICAL(&bleMux);

  if (!hasCommand) return;

  Serial.print("BLE comando: ");
  Serial.println(command);

  if      (!strcmp(command, "MODE:NORMAL"))      currentMode = MODE_NORMAL;
  else if (!strcmp(command, "MODE:CUTE"))        currentMode = MODE_CUTE;
  else if (!strcmp(command, "MODE:FUNNY"))       currentMode = MODE_FUNNY;
  else if (!strcmp(command, "MODE:GRUMPY"))      currentMode = MODE_GRUMPY;
  else if (!strcmp(command, "MODE:SLEEP"))       currentMode = MODE_SLEEP;
  else if (!strcmp(command, "MODE:AUTO"))        currentMode = MODE_AUTOMATIC;
  else if (!strcmp(command, "SCENE:NEXT"))       startScene(chooseSceneForMode());
  else if (!strcmp(command, "SCENE:NORMAL"))     startScene(SCENE_NORMAL);
  else if (!strcmp(command, "SCENE:HAPPY"))      startScene(SCENE_HAPPY);
  else if (!strcmp(command, "SCENE:GRUMPY"))     startScene(SCENE_GRUMPY);
  else if (!strcmp(command, "SCENE:EXCITED"))    startScene(SCENE_EXCITED);
  else if (!strcmp(command, "SCENE:SHY"))        startScene(SCENE_SHY);
  else if (!strcmp(command, "SCENE:CONFUSED"))   startScene(SCENE_CONFUSED);
  else if (!strcmp(command, "SCENE:BORED"))      startScene(SCENE_BORED);
  else if (!strcmp(command, "SCENE:SURPRISED"))  startScene(SCENE_SURPRISED);
  else if (!strcmp(command, "SCENE:SLEEPY"))     startScene(SCENE_SLEEPY);
  else if (!strcmp(command, "SCENE:LAUGHING"))   startScene(SCENE_LAUGHING);
  else if (!strcmp(command, "SCENE:CURIOUS"))    startScene(SCENE_CURIOUS);
  else if (!strcmp(command, "SCENE:DEBOCHADO")) startScene(SCENE_DEBOCHADO);
  else if (!strcmp(command, "SCENE:GARGALHADA")) startScene(SCENE_GARGALHADA);
  else if (!strcmp(command, "SCENE:PISCADINHA")) startScene(SCENE_PISCADINHA);
  else if (!strcmp(command, "SCENE:CONFIANTE")) startScene(SCENE_CONFIANTE);
  else if (!strcmp(command, "SCENE:DUVIDANDO")) startScene(SCENE_DUVIDANDO);
  else if (!strcmp(command, "SCENE:IMPRESSIONADO")) startScene(SCENE_IMPRESSIONADO);
  else if (!strcmp(command, "SCENE:IRRITADO")) startScene(SCENE_IRRITADO);
  else if (!strcmp(command, "SCENE:SIMPATICO")) startScene(SCENE_SIMPATICO);
  else if (!strcmp(command, "SCENE:OLA")) startScene(SCENE_OLA);
  else if (!strcmp(command, "SCENE:OBRIGADO")) startScene(SCENE_OBRIGADO);
  else if (!strcmp(command, "SCENE:FOI_MAL")) startScene(SCENE_FOI_MAL);
  else if (!strcmp(command, "SCENE:CARINHO")) startScene(SCENE_CARINHO);
  else if (!strcmp(command, "MUSIC:NEXT"))       musicStyle = (musicStyle + 1) % 3;
  else if (!strcmp(command, "TRIP:TOGGLE"))      toggleTrip();
  else if (!strcmp(command, "TRIP:RESET"))       resetTrip();
  else if (!strcmp(command, "SCREEN:HOME"))      setScreenRemote(SCREEN_HOME);
  else if (!strcmp(command, "SCREEN:MENU"))      setScreenRemote(SCREEN_MENU);
  else if (!strcmp(command, "SCREEN:MUSIC"))     setScreenRemote(SCREEN_MUSIC);
  else if (!strcmp(command, "SCREEN:ANIMATIONS"))setScreenRemote(SCREEN_ANIMATIONS);
  else if (!strcmp(command, "SCREEN:TRIP"))      setScreenRemote(SCREEN_TRIP);

  if (!strncmp(command, "MODE:", 5)) {
    applyMode();
    startScene(chooseSceneForMode());
  }

  screenNeedsRedraw = true;
  menuNeedsDraw = true;
}

// =====================================================
// ACOES DOS BOTOES
// =====================================================

void menuShortPress() {
  if (currentScreen == SCREEN_MENU) {
    // Dentro do menu, avanca para a proxima opcao.
    menuSelection = (menuSelection + 1) % MENU_COUNT;
    menuNeedsDraw = true;
  } else {
    // Em qualquer tela (Home, Music, Animations ou Trip),
    // abre o menu sem alterar o estado dos outros modos.
    enterMenu();
  }
}

void menuLongPress() {
  if (currentScreen == SCREEN_MENU) {
    // Segurar no menu confirma a opcao selecionada.
    selectMenuItem();
  } else if (currentScreen != SCREEN_HOME) {
    // Segurar fora do menu volta para a tela inicial.
    // Isso nao pausa nem zera o cronometro Trip.
    returnHome();
  } else {
    enterMenu();
  }
}

void animationShortPress() {
  switch (currentScreen) {
    case SCREEN_HOME:
      changeMode();
      break;

    case SCREEN_MENU:
      selectMenuItem();
      break;

    case SCREEN_MUSIC:
      musicStyle = (musicStyle + 1) % 3;
      screenNeedsRedraw = true;
      break;

    case SCREEN_ANIMATIONS:
      startScene(chooseSceneForMode());
      break;

    case SCREEN_TRIP:
      toggleTrip();
      screenNeedsRedraw = true;
      break;
  }
}

void animationLongPress() {
  if (currentScreen == SCREEN_TRIP) {
    resetTrip();
  }
}

// =====================================================
// LEITURA DOS BOTOES - SEM DELAY
// =====================================================

void updateButton(Button &button, bool isMenuButton) {
  bool reading = digitalRead(button.pin);
  unsigned long now = millis();

  if (reading != button.rawState) {
    button.rawState = reading;
    button.lastChange = now;
  }

  if ((unsigned long)(now - button.lastChange) >= DEBOUNCE_MS) {
    if (reading != button.stableState) {
      button.stableState = reading;

      if (button.stableState == LOW) {
        button.pressedAt = now;
        button.longHandled = false;
      } else {
        unsigned long held = now - button.pressedAt;

        if (!button.longHandled) {
          if (isMenuButton) {
            if (held < LONG_PRESS_MS) menuShortPress();
            else menuLongPress();
          } else {
            animationShortPress();
          }
        }
      }
    }
  }

  if (button.stableState == LOW && !button.longHandled) {
    unsigned long held = now - button.pressedAt;

    if (isMenuButton && held >= LONG_PRESS_MS) {
      menuLongPress();
      button.longHandled = true;
    } else if (!isMenuButton &&
               currentScreen == SCREEN_TRIP &&
               held >= TRIP_RESET_HOLD_MS) {
      animationLongPress();
      button.longHandled = true;
    }
  }
}


// =====================================================
// CONTROLE PELO CELULAR - BLUETOOTH LOW ENERGY (BLE)
// =====================================================

void setScreenRemote(ScreenState target) {
  currentScreen = target;
  screenNeedsRedraw = true;

  if (target == SCREEN_MENU) {
    menuSelection = 0;
    menuNeedsDraw = true;
  } else if (target == SCREEN_HOME || target == SCREEN_ANIMATIONS) {
    specialActive = false;
    transitionPending = false;
    applyMode();
    startScene(chooseSceneForMode());
    scheduleNextSpecial();
  }
}

// =====================================================
// SETUP
// =====================================================

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);
  pinMode(BUTTON_ANIM_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("Erro ao iniciar OLED. Verifique endereco e ligacoes.");
    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.display();

  roboEyes.begin(SCREEN_WIDTH, SCREEN_HEIGHT, 60);
  normalEyes();

  roboEyes.setPosition(DEFAULT);
  roboEyes.setMood(DEFAULT);
  roboEyes.setAutoblinker(ON, 3, 2);
  roboEyes.setIdleMode(ON, 2, 2);
  roboEyes.setCuriosity(ON);

  randomSeed(esp_random());

  startupTime = millis();
  startupSequence = true;
  roboEyes.close();

  menuButton.rawState = menuButton.stableState = digitalRead(BUTTON_MENU_PIN);
  animButton.rawState = animButton.stableState = digitalRead(BUTTON_ANIM_PIN);

  Serial.println("Mochi Car V5 BLE iniciado");
  Serial.println("MENU: GPIO18 | ANIMACOES: GPIO19");
  startBleControl();
}

// =====================================================
// LOOP PRINCIPAL
// =====================================================

void loop() {
  processPendingBleCommand();
  sendBleStatus();
  updateButton(menuButton, true);
  updateButton(animButton, false);

  // IMPORTANTE:
  // RoboEyes atualiza e envia o OLED por conta propria.
  // Por isso, nao desenhamos menu/music/trip no mesmo frame
  // em que chamamos roboEyes.update(). Isso elimina a disputa
  // entre dois sistemas escrevendo no mesmo display.

  if (currentScreen == SCREEN_HOME ||
      currentScreen == SCREEN_ANIMATIONS) {

    roboEyes.update();

    if (currentScreen == SCREEN_HOME) {
      updateHomeAnimations();
    } else {
      updateTransition();
      updateSpecialEvent();
      if (!specialActive && !transitionPending) {
        updateSceneMotion();

        if ((long)(millis() - nextSceneTime) >= 0) {
          startScene(chooseSceneForMode());
        }

        if ((long)(millis() - nextSpecialTime) >= 0) {
          startSpecialEvent();
          scheduleNextSpecial();
        }
      }
    }

    return;
  }

  if (currentScreen == SCREEN_MENU) {
    if (menuNeedsDraw) {
      drawMenu();
      menuNeedsDraw = false;
    }
    return;
  }

  if (currentScreen == SCREEN_MUSIC) {
    if (screenNeedsRedraw ||
        (unsigned long)(millis() - lastMusicFrame) >= MUSIC_FRAME_MS) {
      drawMusic();
      lastMusicFrame = millis();
      screenNeedsRedraw = false;
    }
    return;
  }

  if (currentScreen == SCREEN_TRIP) {
    if (screenNeedsRedraw ||
        (unsigned long)(millis() - lastTripFrame) >= TRIP_FRAME_MS) {
      drawTrip();
      lastTripFrame = millis();
      screenNeedsRedraw = false;
    }
    return;
  }
}
