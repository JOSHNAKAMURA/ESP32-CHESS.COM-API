// El codigo podria fallar al ejecutarse en ARDUIDO IDE por la version del ESP32 yo use la 2.0.11 BY ESPRESSOF SYSTEMS 
// USAR PINES DIFERENTES AL 21 Y 22 puden causar problemas de conexión con el LCD por ejemplo al cargar el archivo de la memoria flash el esp32 espera una respuesta del LCD 
// mientras la pantalla aparece vacia

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

// INCIALIZACION DEL LCD 
LiquidCrystal_I2C lcd(0x27, 16, 2); 

//Datos para la conexión via wifi
const char* ssid = "TUSSID, EL NOMBRE DE TU RED WIFI";
const char* password = "LA CONTRASEÑA DE TU RED WIFI";

int estadoInferior = 0;

unsigned long ultimoCambioInferior = 0;

const unsigned long tiempoPantalla = 3000;
// IMPORTANTE EL NOMBRE DEBE IR SIN CAMEL CASE YA QUE LA API LOS USA SIN CAMELCASE ,SI SE PONE DIRECTO NO DARA LOS DATOS CORRECTOS
String playerNickname = "granreichikita";  //GranReiChikita = real
// Textos a desplazar
//String titulo = "    ELO chess.com: GranReiChikita    ";

String titulo = "    ELO chess.com: " + playerNickname + "    ";

// Variales globales para los elos y la concatenacion con su respectivo mensaje
int bulletElo;
int bulletEloBest;

int blitzElo;
int blitzEloBest;

int rapidElo;
int rapidEloBest;

// Posiciones actuales
int posTitulo = 0;
int posMenu = 0;

// Temporizadores
unsigned long ultimoTitulo = 0;
unsigned long ultimoMenu = 0;
// Para la actualizacion de la api
unsigned long ultimaActualizacion = 0;
// 60 segundos el tiempo de actualizacion de la api , no usarlo tan bajo para no saturar el servidor de chesscom y evitar bloqueos
const unsigned long intervaloActualizacion = 60000; 

// Velocidades de desplazamiento del titulo
const unsigned long intervaloTitulo = 300;
const unsigned long intervaloMenu = 200;

// Son los peones mostrados en la pantalla de carga 
byte p0[8] = {
  B00011,
  B00111,
  B01111,
  B01111,
  B01111,
  B00111,
  B00011,
  B00000
};

byte p1[8] = {
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11110,
  B11100,
  B00000
};

byte p2[8] = {
  B00111,
  B01111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte p3[8] = {
  B11100,
  B11110,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte p4[8] = {
  B11111,
  B11111,
  B11111,
  B01111,
  B00111,
  B00011,
  B00001,
  B00000
};

byte p5[8] = {
  B11111,
  B11111,
  B11111,
  B11110,
  B11100,
  B11000,
  B10000,
  B00000
};

byte p6[8] = {
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111,
  B11111
};

byte p7[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

// Metodo para la crecion del peon
void dibujarPeon(int columna) {
  lcd.setCursor(columna, 0);
  lcd.write(byte(0));

  lcd.setCursor(columna + 1, 0);
  lcd.write(byte(1));

  lcd.setCursor(columna, 1);
  lcd.write(byte(2));

  lcd.setCursor(columna + 1, 1);
  lcd.write(byte(3));
}

// Pantalla de carga...
void pantallaInicio() {
  lcd.clear();

  // Aparición secuencial de los peones
  dibujarPeon(0);
  delay(400);

  dibujarPeon(6);
  delay(400);

  dibujarPeon(12);
  delay(400);

  delay(800);

  // Parpadeo
  for (int i = 0; i < 3; i++) {
    lcd.clear();
    delay(150);

    dibujarPeon(0);
    dibujarPeon(6);
    dibujarPeon(12);

    delay(150);
  }

  // Logo despues de los peones
  lcd.clear();

  lcd.setCursor(3, 0);
  lcd.print("CHESS.COM");

  lcd.setCursor(2, 1);
  lcd.print("LOADING...");

  delay(1500);

  // Barrido

  for (int i = 0; i < 16; i++) {
    lcd.scrollDisplayLeft();
    delay(60);
  }

  lcd.clear();
}

// Linea inferior que mostrara los datos del elo
void actualizarLineaInferior() {
  unsigned long ahora = millis();

  if (ahora - ultimoCambioInferior < tiempoPantalla)
    return;

  ultimoCambioInferior = ahora;

  lcd.setCursor(0, 1);
  lcd.print("                ");

  lcd.setCursor(0, 1);
  // Switch de manejo de estados , permite coexistir en armonia con el titulo dinamico de bienvenida
  // cada case es un estado
  switch (estadoInferior) {
    case 0:
      lcd.print("BULLET ELO");
      break;

    case 1:
      lcd.print("Current:");
      lcd.print(bulletElo);
      break;

    case 2:
      lcd.print("Best:");
      lcd.print(bulletEloBest);
      break;

    case 3:
      lcd.print("BLITZ ELO");
      break;

    case 4:
      lcd.print("Current:");
      lcd.print(blitzElo);
      break;

    case 5:
      lcd.print("Best:");
      lcd.print(blitzEloBest);
      break;

    case 6:
      lcd.print("RAPID ELO");
      break;

    case 7:
      lcd.print("Current:");
      lcd.print(rapidElo);
      break;

    case 8:
      lcd.print("Best:");
      lcd.print(rapidEloBest);
      break;
  }

  estadoInferior++;

  if (estadoInferior > 8) {
    estadoInferior = 0;
  }
}
// Manejo de la conexion wifi
void conexionWifi() {
  WiFi.begin(ssid, password);


  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Sin conexión a internet");
  }
  Serial.println("Conectado a internet");
}

void realizarPeticion() {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  //ruta de la api (puede cambiar) y el nicknamePlayer tiene que estar sin CAMELCASE ya que chesscom lo maneja asi.
  http.begin(
    client,
    "https://api.chess.com/pub/player/" + playerNickname + "/stats");

  int codigo = http.GET();
  
  Serial.print("Codigo HTTP: ");
  Serial.println(codigo);
  
  Serial.print("Location: ");
  Serial.println(http.header("Location"));

  String respuesta = http.getString();
  Serial.println(respuesta);

  // Manejador del json con la libreria ArduinoJSON mas adelante se vera su funcionamiento
  DynamicJsonDocument doc(16384);
  deserializeJson(doc, respuesta);


  // Destructuración (como en js) del json con los stats ,doc hace la magia
  bulletElo =
    doc["chess_bullet"]["last"]["rating"];
  Serial.println(bulletElo);

  bulletEloBest =
    doc["chess_bullet"]["best"]["rating"];
  Serial.println(bulletEloBest);

  //Elo Blitz mejor y peor
  blitzElo =
    doc["chess_blitz"]["last"]["rating"];
  Serial.println(blitzElo);

  blitzEloBest =
    doc["chess_blitz"]["best"]["rating"];
  Serial.println(blitzEloBest);

  //Elo Rapid mejor y peor
  rapidElo =
    doc["chess_rapid"]["last"]["rating"];
  Serial.println(rapidElo);

  rapidEloBest =
    doc["chess_rapid"]["best"]["rating"];
  Serial.println(rapidEloBest);

  http.end();
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  // El pin 21 es el SDA Y EL PIN 22 ES EL SCL DEL ESP32 PUEDE VARIAR
  Wire.begin(21, 22); 
  Wire.setClock(100000);

  lcd.init();
  lcd.backlight();

  lcd.createChar(0, p0);
  lcd.createChar(1, p1);
  lcd.createChar(2, p2);
  lcd.createChar(3, p3);
  lcd.createChar(4, p4);
  lcd.createChar(5, p5);
  lcd.createChar(6, p6);
  lcd.createChar(7, p7);
  pantallaInicio();

  conexionWifi();
  realizarPeticion();
}

void loop() {
  unsigned long ahora = millis();
  // Actualizar API cada minuto
  if (ahora - ultimaActualizacion >= intervaloActualizacion) {
    ultimaActualizacion = ahora;
    realizarPeticion();
  }


  // TITULO DINAMICO : "Elo chess.com : GranReyChikita"
  if (ahora - ultimoTitulo >= intervaloTitulo) {
    ultimoTitulo = ahora;

    String ventanaTitulo = titulo.substring(posTitulo);

    while (ventanaTitulo.length() < 16)
      ventanaTitulo += titulo;

    lcd.setCursor(0, 0);
    lcd.print(ventanaTitulo.substring(0, 16));

    posTitulo++;

    if (posTitulo >= titulo.length())
      posTitulo = 0;
  }
  // Linea con los datos del jugador bala, blitz , rapidas.
  actualizarLineaInferior();
}
