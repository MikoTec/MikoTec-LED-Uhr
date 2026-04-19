const char root_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html><head>
  <meta http-equiv=Content-Type content='text/html; charset=utf-8' />
  <meta name=viewport content='width=device-width, initial-scale=1.0'>
<script>
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);
connection.onopen = function () {  connection.send('Connect ' + new Date()); };
connection.onerror = function (error) {    console.log('WebSocket Error ', error);};
connection.onmessage = function (e) {  console.log('Server: ', e.data);};
</script>
$externallinks
$csswgradient
</head>
<body class=settings-page>
  <div style="text-align:center;background:#fff;padding:10px 0 5px 0;position:relative;z-index:10;">
    <h1 style="margin:0;font-size:1.6em;color:#333;font-family:Abel,sans-serif;">MikoTec's LED Uhr</h1>
    <p style="margin:2px 0 0 0;font-size:0.9em;color:#888;">Version $firmware_version</p>
  </div>
  <div id='canvasholder'>
    <canvas id='canvas'></canvas>
  </div>
  <form action=/ method=GET>
    <div class="color-section">
      <div class="color-box">
        <label>Stundenfarbe</label>
        <input type='color' name = 'hourcolorspectrum' id='hourcolorspectrum' value='$hourcolor'/>
        <input type='hidden' name = 'hourcolor' id = 'hourcolor' value = '$hourcolor'>
      </div>
      <div class="color-box">
        <label>Minutenfarbe</label>
        <input type='color' name='minutecolorspectrum' id='minutecolorspectrum' value='$minutecolor'/>
        <input type='hidden' name = 'minutecolor' id = 'minutecolor' value = '$minutecolor'/>
      </div>
    </div>
    <div class="slide-section">
      <div class="point-slide">
        <label>Überblendeffekt</label>
        <input type='range' id= 'blendpoint' name='blendpoint' min='0' max='255' value=$blendpoint>
      </div>
      <div class="point-slide">
        <label>Helligkeit</label>
        <input type='range' id= 'brightness' name='brightness' min='10' max=$maxBrightness value=$brightness>
      </div>
    </div>
    <div class="btn-box scheme1">
      <input class='btn btn-sm' type='submit' name='submit' value='Speichern V1' />
      <input class='btn btn-sm' type='submit' name='submit' value='Laden V1' />
    </div>
    <div class="btn-box scheme2">
      <input class='btn btn-sm' type='submit' name='submit' value='Speichern V2' />
      <input class='btn btn-sm' type='submit' name='submit' value='Laden V2' />
    </div>
    <div class="btn-box scheme3">
      <input class='btn btn-sm' type='submit' name='submit' value='Speichern V3' />
      <input class='btn btn-sm' type='submit' name='submit' value='Laden V3' />
    </div>
    <div class="btn-box scheme4">
      <input class='btn btn-sm' type='submit' name='submit' value='Speichern V4' />
      <input class='btn btn-sm' type='submit' name='submit' value='Laden V4' />
    </div>
    <div class="btn-box">
    $alarm
    </div>
    <div class="btn-footer">
      <a class="btn btn-default" href=/settings>Einstellungen</a>
      <a class="btn btn-default" href=/hilfe>Hilfe</a>
      <input class='btn btn-green' type=submit name=submit value='Update'/>
    </div>
  </form>
<div class="bottomspacer">
</body>
</html>
)=====";
