const char timezone_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head><title>Zeitzone</title>
<meta name=viewport content="width=device-width, initial-scale=1">
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<link rel=stylesheet href=https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/css/bootstrap.min.css>
<script src=https://ajax.googleapis.com/ajax/libs/jquery/1.11.3/jquery.min.js></script>
<script src=https://maxcdn.bootstrapcdn.com/bootstrap/3.3.5/js/bootstrap.min.js></script>
<link rel=stylesheet href='clockmenustyle.css'>
<style>
.container{max-width:600px;margin:0 auto;padding:15px;}
.tab-content{padding:15px 0;}
.nav-tabs{margin-bottom:15px;}
.nav-tabs li{display:inline-block;}
.nav-tabs li a{padding:8px 15px;text-decoration:none;border:1px solid #ddd;margin-right:5px;color:#333;}
.nav-tabs li.active a{background:#fff;border-bottom:1px solid #fff;font-weight:bold;}
.tab-pane{display:none;}
.tab-pane.active{display:block;}
input[type=text]{padding:6px;margin:5px 0;font-size:16px;width:200px;}
input[type=submit]{padding:8px 16px;margin:10px 0;font-size:14px;cursor:pointer;}
h2{text-align:center;}
h3{margin-top:10px;}
.btn-box{text-align:center;margin-top:15px;}
.btn{display:inline-block;padding:10px 20px;margin:5px;text-decoration:none;border:1px solid #ccc;color:#333;background:#f5f5f5;text-transform:uppercase;font-size:12px;letter-spacing:1px;}
</style>
</head>
<body class="settings-page">
<div class=container>
<h2>Zeitzone einstellen</h2>
<ul class="nav nav-tabs" id="tztabs">
<li class=active><a href="#" onclick="showTab('GPS');return false;">Suche</a></li>
<li><a href="#" onclick="showTab('Manual');return false;">Manuell</a></li>
<li><a href="#" onclick="showTab('City');return false;">Stadt</a></li>
</ul>
<div class=tab-content>
<div id=GPS class="tab-pane active">
<h3>Standort</h3>
<div style="margin-bottom:10px;">
<input type=text id=searchAddr placeholder="Adresse oder Stadt eingeben..." style="width:100%;padding:8px;font-size:16px;box-sizing:border-box;">
<button type=button onclick=searchLocation() style="margin-top:5px;padding:8px 16px;cursor:pointer;">Suchen</button>
<span id=searchStatus style="margin-left:10px;color:#888;"></span>
</div>
<form action=/ method=GET>
Breitengrad:<input type=text name=latitude id=latitude value=$latitude><br>
Laengengrad:<input type=text name=longitude id=longitude value=$longitude><br>
<input type=submit name=submit value='Update Timezone'/></form>
</div>
<div id=Manual class="tab-pane">
<form action=/ method=GET>
UTC Versatz <input type=text name=timezone id=timezone value=$timezone><br>
<input type=submit name=submit value='Update Timezone'/></form>
</div>
<div id=City class="tab-pane">
<h3>Stadt waehlen</h3>
<form action=/ method=GET>
<select id=citysel onchange="cityChanged()" style="font-size:16px;padding:6px;width:100%;margin:8px 0">
<option value="">-- Stadt waehlen --</option>
<option value="52.52,13.41">Berlin</option>
<option value="48.14,11.58">Muenchen</option>
<option value="53.55,9.99">Hamburg</option>
<option value="50.94,6.96">Koeln</option>
<option value="50.11,8.68">Frankfurt</option>
<option value="48.78,9.18">Stuttgart</option>
<option value="51.23,6.78">Duesseldorf</option>
<option value="51.51,7.47">Dortmund</option>
<option value="51.45,7.01">Essen</option>
<option value="51.17,7.08">Solingen</option>
<option value="51.05,7.00">Wuppertal</option>
<option value="51.34,12.37">Leipzig</option>
<option value="51.05,13.74">Dresden</option>
<option value="48.40,10.00">Ulm</option>
<option value="49.45,11.08">Nuernberg</option>
<option value="47.37,8.54">Zuerich</option>
<option value="48.21,16.37">Wien</option>
<option value="46.95,7.45">Bern</option>
<option value="51.50,-0.13">London</option>
<option value="48.86,2.35">Paris</option>
<option value="40.42,-3.70">Madrid</option>
<option value="41.90,12.50">Rom</option>
<option value="52.37,4.90">Amsterdam</option>
<option value="59.33,18.07">Stockholm</option>
<option value="55.68,12.57">Kopenhagen</option>
<option value="60.17,24.94">Helsinki</option>
<option value="38.72,-9.14">Lissabon</option>
<option value="50.08,14.44">Prag</option>
<option value="47.50,19.04">Budapest</option>
<option value="52.23,21.01">Warschau</option>
<option value="40.71,-74.01">New York</option>
<option value="34.05,-118.24">Los Angeles</option>
<option value="35.69,139.69">Tokyo</option>
<option value="-33.87,151.21">Sydney</option>
</select>
<input type=text name=latitude id=citylat style="display:none">
<input type=text name=longitude id=citylng style="display:none">
<input type=submit name=submit value='Update Timezone'/></form>
</div>
</div>
</div>
<div class=btn-box style="margin-top:15px">
</div>
<script>
var x=document.getElementById("latitude");var y=document.getElementById("longitude");
function searchLocation(){
var addr=document.getElementById("searchAddr").value;
if(!addr){document.getElementById("searchStatus").innerText="Bitte Adresse eingeben";return;}
document.getElementById("searchStatus").innerText="Suche...";
var xhr=new XMLHttpRequest();
xhr.open("GET","https://nominatim.openstreetmap.org/search?q="+encodeURIComponent(addr)+"&format=json&limit=1",true);
xhr.onreadystatechange=function(){
if(xhr.readyState==4){
if(xhr.status==200){
var res=JSON.parse(xhr.responseText);
if(res.length>0){
x.value=Math.round(parseFloat(res[0].lat)*100)/100;
y.value=Math.round(parseFloat(res[0].lon)*100)/100;
document.getElementById("searchStatus").innerText="Gefunden: "+res[0].display_name.substring(0,50)+"...";
}else{document.getElementById("searchStatus").innerText="Nichts gefunden.";}
}else{document.getElementById("searchStatus").innerText="Fehler bei der Suche.";}
}};
xhr.send();}
function cityChanged(){var s=document.getElementById("citysel").value;if(s!=""){var p=s.split(",");document.getElementById("citylat").value=p[0];document.getElementById("citylng").value=p[1];document.getElementById("latitude").value=p[0];document.getElementById("longitude").value=p[1];}}
function showTab(id){var panes=document.getElementsByClassName("tab-pane");for(var i=0;i<panes.length;i++){panes[i].className="tab-pane";}document.getElementById(id).className="tab-pane active";var tabs=document.querySelectorAll(".nav-tabs li");for(var i=0;i<tabs.length;i++){tabs[i].className="";}event.target.parentElement.className="active";}
</script>
</body>
</html>
)=====";
