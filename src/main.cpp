#include <ILI9341_t3.h>
#include <iostream>
#include <string>
#include <XPT2046_Touchscreen.h>
#include <Bounce2.h>
#include <list>
#include <RF24.h>
#include <RF24Network.h>

//display
#define TFT_DC 9
#define TFT_CS 10
#define TFT_RST 255
#define TFT_MOSI 11
#define TFT_SCLK 13
#define TFT_MISO 12
#define TOUCH_CS 8
#define TOUCH_IRQ 2
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
bool tsSuccess = false;

//buttons
constexpr int BTN_1_PIN = 24;
constexpr int BTN_2_PIN = 28;
constexpr int BTN_3_PIN = 29;
Bounce debouncer_1;
Bounce debouncer_2;
Bounce debouncer_3;

//buzzer
constexpr int BUZZ_PIN = 25;

//radio
constexpr uint8_t CE_PIN = 22;
constexpr uint8_t CSN_PIN = 19;
RF24 radio (CE_PIN, CSN_PIN);
RF24Network network(radio);
const uint16_t myAddr = 00; //TODO: CHANGE FOR EACH ONE
const uint8_t networkChannel = 90; //not unique
struct PingDataPackage {
    uint32_t message_id;
    float sensor_value; //todo: do I need this?
    char text_data[16];
};

//etc global vars
bool atHome = false; //if at home screen, for buttons
bool atArt = false; //if at art screen, for buttons
bool artEnabled = false; //to enable touch screen art functions
std::list<std::pair<int16_t,int16_t>> artStoredValues = {}; //saved positions for art

struct ButtonImage {
    int16_t textSize;
    int16_t x,y;
    int16_t w,h;
    uint16_t fillColor;
    int16_t textX, textY;
};
ButtonImage unlockButton = {
    2,
    10,100,
    70,200,
    ILI9341_BLUE,
    11,115
};
ButtonImage unlockButtonFull = {
    2,
    10,0,
    70,300,
    ILI9341_BLUE,
    11,115
};
ButtonImage unlockButtonMini = {
    2,
    10,215,
    70,200,
    ILI9341_BLUE,
    11,220
};
ButtonImage pingButton = {
    2,
    85,100,
    70,200,
    ILI9341_YELLOW,
    86, 115
};
ButtonImage pingButtonFull = {
    2,
    10,0,
    70,300,
    ILI9341_YELLOW,
    86, 115
};
ButtonImage pingButtonMini = {
    2,
    85,215,
    70,200,
    ILI9341_YELLOW,
    86,220
};
ButtonImage artButton = {
    2,
    160,100,
    70,200,
    ILI9341_RED,
    161, 115
};
ButtonImage artButtonFull = {
    2,
    10,0,
    70,300,
    ILI9341_RED,
    161, 115
};
ButtonImage artButtonMini = {
    2,
    160,215,
    70,200,
    ILI9341_RED,
    161,220
};

void drawButton(const ButtonImage &button, const std::string& text) {
    tft.fillRect(button.x,button.y,button.w,button.h,button.fillColor);

    tft.setCursor(button.textX,button.textY);
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(button.textSize);
    tft.print(text.c_str());
}

//display text one letter at a time
void slow_print(const std::string& msg, const int delay_time) {
    for (size_t i = 0; msg[i] != '\0'; ++i) {
        tft.print(msg[i]);
        delay(delay_time);
    }
}

constexpr char LOADING_CONST[] = {'/','-','\\','|'};
void loadingAnim(const char *text, const int rotations, const int16_t cursorX, const int16_t cursorY) {
    for (int j = 0; j < rotations; ++j) {
        for (char i : LOADING_CONST) {
            tft.setCursor(cursorX,cursorY);
            tft.print(text);
            tft.print(" ");
            tft.print(i);
            delay(200);
        }
    }
    //gets rid of symbol
    tft.setCursor(cursorX,cursorY);
    tft.print(text);
    tft.print("  ");
    tft.setCursor(cursorX,cursorY);
    tft.print(text); //important to do it again to get cursor in correct spot bc the space
}

void displayHome() {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.println(" Horizonless_PDA v0.3");
    delay(200);
    tone(BUZZ_PIN,1200,200);
    tft.print(" . ");
    if (tsSuccess)
        tft.print("touch screen success \n");
    else
        tft.print("touch screen FAILURE \n");
    delay(200);
    tone(BUZZ_PIN,1200,200);
    tft.print(" . ");
    if (radio.isChipConnected())
        tft.print("radio success \n");
    else
        tft.print("radio FAILURE \n");
    delay(200);
    tone(BUZZ_PIN,800,200);
    tft.print("\n ");
    slow_print("Select an action:\n", 15);

    //draw buttons
    drawButton(unlockButton, "Unlock");
    drawButton(pingButton, "Ping");
    drawButton(artButton, "Draw");

    atHome = true;
}

void reset() {
    atArt = false;

    tone(BUZZ_PIN,500,300);
    artEnabled = false;
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    tft.fillScreen(ILI9341_BLACK);
    loadingAnim("Resetting... ", 2, 75,100);
    displayHome();
}

void unlockMenu() {
    atHome = false; //to change button behavior
    tone(BUZZ_PIN,1000,200);
    tft.fillScreen(ILI9341_BLACK);
    drawButton(unlockButtonFull, "");
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    loadingAnim("not implemented", 0, 88, 25);

    delay(3000);
    displayHome();
}

void pingMenu() {
    atHome = false; //to change button behavior
    tone(BUZZ_PIN,1000,200);
    /*slow_print("\n Sending Message...", 30);
    //radio
    const char msg[] = "You Got Mail :O";
    bool ok = radio.write(&msg, sizeof(msg));
    tone(BUZZ_PIN,(ok ? (1200) : (600)),200);
    if (ok)
        slow_print("SUCCESS", 30);
    else {
        slow_print("FAILED", 30);
        radio.printDetails();
    }*/
    tft.fillScreen(ILI9341_BLACK);
    drawButton(pingButtonFull, "");
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    loadingAnim("checking radio", 2, 88, 25);
    loadingAnim("pinging devices", 8, 88, 50);
    loadingAnim("processing", 2, 88, 75);
    tft.setCursor(88,100);
    tft.print("2 devices pinged");
    delay(3000);
    displayHome();
}

void artMenu() {
    atHome = false; //to change button behavior
    tone(BUZZ_PIN,1000,200);
    tft.fillScreen(ILI9341_BLACK);
    drawButton(artButtonFull, "");
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    loadingAnim("connecting", 4, 88, 25);
    tft.fillScreen(ILI9341_BLACK);

    artEnabled = true;
    tft.fillScreen(ILI9341_DARKGREY);

    //load saved art
    for (std::pair<int16_t,int16_t> value : artStoredValues) {
        tft.fillCircle(value.first, value.second, 3, ILI9341_BLUE);
    }

    //draw buttons
    drawButton(unlockButtonMini, "Clear");
    drawButton(pingButtonMini, "");
    drawButton(artButtonMini, "Back");

    atArt = true;
}
void clearArt() {
    tone(BUZZ_PIN,400,600);
    artStoredValues.clear();
    tft.fillScreen(ILI9341_DARKGREY);
    //draw buttons
    drawButton(unlockButtonMini, "Clear");
    drawButton(pingButtonMini, "");
    drawButton(artButtonMini, "Back");
}

void setup() {
    Serial.begin(9600);

    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(ILI9341_WHITE);
    tft.setTextColor(ILI9341_BLACK); //for initial anim
    tft.setTextSize(2);
    tft.setCursor(60,100);
    slow_print("Initializing...", 30);

    //buttons
    pinMode(BTN_1_PIN, INPUT_PULLUP);
    pinMode(BTN_2_PIN, INPUT_PULLUP);
    pinMode(BTN_3_PIN, INPUT_PULLUP);
    debouncer_1.attach(BTN_1_PIN);
    debouncer_2.attach(BTN_2_PIN);
    debouncer_3.attach(BTN_3_PIN);
    debouncer_1.interval(5);
    debouncer_2.interval(5);
    debouncer_3.interval(5);

    //buzzer
    pinMode(BUZZ_PIN, OUTPUT);

    //radio todo: add serial debug
    SPI.begin();
    radio.begin();
    radio.setPALevel(RF24_PA_LOW);
    radio.setDataRate(RF24_250KBPS);
    network.begin(networkChannel, myAddr);

    //initialize touch screen
    if (ts.begin())
        tsSuccess = true;
    else
        tsSuccess = false;

    delay(300);
    tft.setTextColor(ILI9341_WHITE,ILI9341_BLACK);
    tft.fillScreen(ILI9341_BLACK);
    loadingAnim("Initializing...", 4, 60,100);
    displayHome();
}

unsigned long lastTouchTime = 0;
constexpr unsigned long debounceDelay = 5;

void loop() {
    //buttons
    debouncer_1.update();
    debouncer_2.update();
    debouncer_3.update();
    bool btn1Pressed = debouncer_1.fell();
    bool btn2Pressed = debouncer_2.fell();
    bool btn3Pressed = debouncer_3.fell();
    if (btn1Pressed) {
        if (atHome)
            unlockMenu();
        else if (atArt)
            clearArt();
        else
            tone(BUZZ_PIN,200,200);
    }
    else if (btn2Pressed) {
        if (atHome)
            pingMenu();
        else
            tone(BUZZ_PIN,200,200);
    }
    else if (btn3Pressed) {
        if (atHome)
            artMenu();
        else if (atArt)
            reset();
        else
            tone(BUZZ_PIN,200,200);
    }

    //art
    if (artEnabled) {
        if (ts.touched()){
            unsigned long currentTime = millis();
            if (currentTime - lastTouchTime > debounceDelay){
                const TS_Point p = ts.getPoint();
                lastTouchTime = currentTime;

                const auto x = static_cast<int16_t>(map(p.x, 3900, 414, 0, tft.width()));
                const auto y = static_cast<int16_t>(map(p.y, 3630, 449, 0, tft.height()));

                artStoredValues.emplace_back(x,y); //save value

                tft.fillCircle(x, y, 3, ILI9341_BLUE);
            }
        }
    }
}