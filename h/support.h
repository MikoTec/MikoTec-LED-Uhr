const char support_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<title>Support</title>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
$externallinks
</head>
<body class=settings-page>
$menu

<label class="section-head">Support - Systemprotokoll</label>

<div id="rcorners2" style="margin-bottom:15px">
<form class=form-verticle>
<ul>
<li>
<label>Firmware</label>
<div class=form-field id="fwver">--</div>
</li>
<li>
<label>Freier Speicher</label>
<div class=form-field id="freemem">--</div>
</li>
<li>
<label>Betriebszeit</label>
<div class=form-field id="uptime">--</div>
</li>
</ul>
</form>
</div>

<div id="rcorners2">
<label class="section-head">Serielles Protokoll</label>
<div style="margin:10px 0;display:flex;flex-wrap:wrap;gap:8px;align-items:center;">
<button class="btn btn-sm" onclick="refreshLog()">Aktualisieren</button>
<button class="btn btn-sm" onclick="downloadLog()">Herunterladen</button>
<button class="btn btn-sm" onclick="clearLogView()">Leeren</button>
<label style="margin:0 4px 0 12px;font-size:13px;">Intervall:</label>
<select id="logInterval" onchange="updateInterval()" style="padding:4px 8px;border-radius:4px;border:1px solid #ccc;font-size:13px;">
  <option value="5000">5 Sek</option>
  <option value="10000">10 Sek</option>
  <option value="30000">30 Sek</option>
  <option value="60000" selected>1 Min</option>
  <option value="300000">5 Min</option>
  <option value="0">Aus</option>
</select>
</div>
<textarea id="logbox" readonly style="width:100%;height:300px;font-family:monospace;font-size:12px;background:#111;color:#0f0;border:1px solid rgba(0,0,0,0.15);border-radius:3px;padding:8px;resize:vertical"></textarea>
</div>

<script>
function refreshLog(){
var x=new XMLHttpRequest();
x.onreadystatechange=function(){
if(x.readyState==4&&x.status==200){
document.getElementById("logbox").value=x.responseText;
var b=document.getElementById("logbox");
b.scrollTop=b.scrollHeight;
}};
x.open("GET","/getlog",true);
x.send();
}
function refreshInfo(){
var x=new XMLHttpRequest();
x.onreadystatechange=function(){
if(x.readyState==4&&x.status==200){
var d=JSON.parse(x.responseText);
document.getElementById("fwver").innerHTML=d.fw;
document.getElementById("freemem").innerHTML=d.heap+" Bytes";
var s=Math.floor(d.up/1000);
var m=Math.floor(s/60);var h=Math.floor(m/60);
document.getElementById("uptime").innerHTML=h+"h "+m%60+"m "+s%60+"s";
}};
x.open("GET","/getsysinfo",true);
x.send();
}
function downloadLog(){
var t=document.getElementById("logbox").value;
var b=new Blob([t],{type:"text/plain"});
var a=document.createElement("a");
a.href=URL.createObjectURL(b);
a.download="lightclock_log.txt";
a.click();
}
function clearLogView(){
document.getElementById("logbox").value="";
}
function rebootClock(){
if(confirm("Uhr wirklich neu starten?")){
var x=new XMLHttpRequest();
x.open("GET","/reboot",true);
x.send();
alert("Uhr wird neu gestartet...");
}
}
var logTimer = null;
var infoTimer = null;
function updateInterval(){
  var val = parseInt(document.getElementById("logInterval").value);
  if(logTimer) clearInterval(logTimer);
  if(val > 0) logTimer = setInterval(refreshLog, val);
}
refreshLog();
refreshInfo();
logTimer = setInterval(refreshLog, 60000);
infoTimer = setInterval(refreshInfo, 10000);
</script>



</body>
</html>
)=====";
