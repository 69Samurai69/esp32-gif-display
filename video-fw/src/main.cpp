#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// LILYGO/TTGO T-Display V1.1: classic ESP32 + ST7789.
static const char* AP_SSID = "TTGO-Video";
static const char* AP_PASS = "ttgovideo";

TFT_eSPI tft;
WebServer http(80);
WebSocketsServer ws(81);
uint32_t lastFrameRequest = 0;
bool frameBusy = false;

static const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="ru"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>TTGO Video</title>
<style>
*{box-sizing:border-box}
body{font:16px system-ui;background:#10141b;color:#f3f5f7;margin:0;padding:18px}
main{max-width:520px;margin:auto}h2{margin:0 0 8px}p{color:#bdc5d0}
button,input{font:inherit;margin:8px 0;padding:12px;border-radius:10px;border:0}
button{background:#3b82f6;color:white;font-weight:600}input{background:#202a36;color:white;width:100%}
video{display:block;width:100%;background:#000;border-radius:8px;margin-top:12px}
#preview{display:block;width:100%;max-width:480px;aspect-ratio:16/9;image-rendering:pixelated;background:#000;border:1px solid #465160;border-radius:8px}
#status{padding:10px;background:#202a36;border-radius:8px;overflow-wrap:anywhere}
small{color:#bdc5d0}
</style><main>
<h2>TTGO Video Stream</h2>
<p>Подключись к Wi-Fi <b>TTGO-Video</b> (пароль <b>ttgovideo</b>) и открой <b>http://192.168.4.1</b>. Выбери видео — страница автоматически обрежет его под экран 240×135.</p>
<input id="file" type="file" accept="video/*">
<br><button id="start">Начать трансляцию</button><button id="stop">Остановить</button>
<div id="status">Подключение к дисплею…</div>
<video id="video" playsinline controls></video>
<p><small>Предпросмотр кадра после обрезки (точно в пропорции экрана):</small></p>
<canvas id="canvas" width="240" height="135"></canvas>
<p><small>Кадры уменьшаются до 240×135 и сжимаются в JPEG перед отправкой. Звук не передаётся.</small></p>
</main>
<script>
const statusEl=document.getElementById('status');
const fileEl=document.getElementById('file');
const video=document.getElementById('video');
const canvas=document.getElementById('canvas');
const ctx=canvas.getContext('2d',{alpha:false});
ctx.imageSmoothingEnabled=true;
ctx.imageSmoothingQuality='high';
let socket=null, active=false, sending=false, objectUrl=null;
function status(s){statusEl.textContent=s;}
function drawFrame(){
  const vw=video.videoWidth,vh=video.videoHeight;
  if(!vw||!vh)return false;
  // Fill the whole 240x135 screen: crop excess edges instead of adding black bars.
  const scale=Math.max(240/vw,135/vh);
  const w=vw*scale,h=vh*scale;
  ctx.fillStyle='#000';ctx.fillRect(0,0,240,135);
  ctx.drawImage(video,(240-w)/2,(135-h)/2,w,h);
  return true;
}
function connect(){
  socket=new WebSocket('ws://'+location.hostname+':81/');
  socket.binaryType='arraybuffer';
  socket.onopen=()=>status('Подключено. Выбери видео и нажми «Начать трансляцию».');
  socket.onclose=()=>{status('Соединение закрыто. Обнови страницу для переподключения.');active=false;};
  socket.onerror=()=>status('Ошибка WebSocket-соединения.');
  socket.onmessage=async e=>{
    if(e.data!=='requestFrame' || !active || sending || video.readyState<2 || video.paused || video.ended) return;
    sending=true;
    try{
      if(!drawFrame())return;
      const blob=await new Promise(resolve=>canvas.toBlob(resolve,'image/jpeg',0.48));
      if(blob && socket && socket.readyState===WebSocket.OPEN && active){
        socket.send(await blob.arrayBuffer());
      }
    }catch(err){status('Ошибка кадра: '+err.message);}
    finally{sending=false;}
  };
}
fileEl.addEventListener('change',()=>{
  if(objectUrl)URL.revokeObjectURL(objectUrl);
  const f=fileEl.files&&fileEl.files[0]; if(!f)return;
  objectUrl=URL.createObjectURL(f);video.src=objectUrl;video.load();
  video.onloadeddata=()=>{drawFrame();status('Видео выбрано: '+f.name+'. Проверь предпросмотр и нажми «Начать трансляцию».');};
});
video.addEventListener('seeked',drawFrame);
document.getElementById('start').onclick=async()=>{
  if(!fileEl.files||!fileEl.files[0]){status('Сначала выбери видео на телефоне.');return;}
  try{await video.play();active=true;status('Передача видео на экран TTGO…');}
  catch(e){status('Браузер заблокировал воспроизведение. Нажми Play на видео и затем начни трансляцию.');}
};
document.getElementById('stop').onclick=()=>{active=false;video.pause();status('Трансляция остановлена.');};
connect();
</script></html>
)HTML";

bool tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (x >= tft.width() || y >= tft.height()) return true;
  tft.pushImage(x, y, w, h, bitmap);
  return true;
}

void onWebSocketEvent(uint8_t client, WStype_t type, uint8_t* payload, size_t length) {
  if (type != WStype_BIN || !payload || length < 4 || frameBusy) return;
  frameBusy = true;
  TJpgDec.drawJpg(0, 0, payload, (uint32_t)length);
  frameBusy = false;
}

void setup() {
  Serial.begin(115200);
  delay(150);
  tft.init();
  tft.setRotation(1); // landscape: 240x135
  // Keep byte swapping in TFT_eSPI only; swapping in both TFT_eSPI and TJpg_Decoder
  // can cancel out and produce incorrect colors.
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("TTGO Video", tft.width()/2, 35, 2);
  tft.drawString("Starting Wi-Fi...", tft.width()/2, 62, 2);

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tftOutput);
  // Do not call TJpgDec.setSwapBytes(true): TFT_eSPI handles byte swapping above.

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());

  http.on("/", HTTP_GET, []() {
    http.send_P(200, "text/html; charset=utf-8", PAGE);
  });
  http.onNotFound([]() {
    http.sendHeader("Location", "/", true);
    http.send(302, "text/plain", "");
  });
  http.begin();
  ws.begin();
  ws.onEvent(onWebSocketEvent);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Wi-Fi: TTGO-Video", tft.width()/2, 36, 2);
  tft.drawString("Pass: ttgovideo", tft.width()/2, 61, 2);
  tft.drawString("Open 192.168.4.1", tft.width()/2, 88, 2);
}

void loop() {
  http.handleClient();
  ws.loop();
  // ~12.5 fps is a more realistic target for decoding and drawing JPEG frames on classic ESP32.
  if (millis() - lastFrameRequest >= 80) {
    lastFrameRequest = millis();
    ws.broadcastTXT("requestFrame");
  }
  delay(1);
}
