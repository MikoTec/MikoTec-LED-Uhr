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
<div style="margin:10px 0">
<button class="btn btn-sm" onclick="refreshLog()">Aktualisieren</button>
<button class="btn btn-sm" onclick="downloadLog()">Herunterladen</button>
<button class="btn btn-sm" onclick="clearLogView()">Leeren</button>
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
refreshLog();
refreshInfo();
setInterval(refreshLog,5000);
setInterval(refreshInfo,10000);
</script>

<div class=btn-box>
<a class="btn btn-default" href=/hilfe>Zurueck zur Hilfe</a>
<button class="btn" onclick="rebootClock()" style="background:#c00;color:#fff">Neustart</button>
</div>

</body>
</html>
)=====";
