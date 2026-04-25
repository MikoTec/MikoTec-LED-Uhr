// Einstellungen JavaScript
function updateTime(){
var x=new XMLHttpRequest();
x.onreadystatechange=function(){
if(x.readyState==4&&x.status==200){
document.getElementById("esptime").innerHTML=x.responseText;
}};
x.open("GET","/gettime",true);
x.send();
}
function setManualTime(){
var t=document.getElementById("mantime").value;
if(t==""){return;}
var x=new XMLHttpRequest();
x.onreadystatechange=function(){
if(x.readyState==4&&x.status==200){
updateTime();
}};
x.open("GET","/timeset?time="+t,true);
x.send();
}
setInterval(updateTime,1000);
updateTime();
var nb=document.querySelector('input[name=nightbrightness]');
nb.oninput=function(){document.getElementById('nbval').innerText=this.value+'%';};
function toggleSleepFields(){
var v=document.getElementById('autosleep').value;
document.getElementById('manualsleep').style.display=(v=='0')?'block':'none';
}
toggleSleepFields();
