// Support/Log JavaScript
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
