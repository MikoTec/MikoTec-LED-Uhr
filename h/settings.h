const char settings_html[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
$externallinks
<script type="text/javascript">
function CheckClockType(val){
 var pixcount=document.getElementById('pixelCount');
 var power=document.getElementById('powerType');


 switch(val) {
   case '1':
    document.getElementById('pixelCountli').style.display='none';
    document.getElementById('powerTypeli').style.display='none';
    pixcount.value=120;
    power.value=1;
    break;
  case '2':
    document.getElementById('pixelCountli').style.display='none';
    document.getElementById('powerTypeli').style.display='none';
    pixcount.value=60;
    power.value=2;
    break;
  case '3':
    document.getElementById('pixelCountli').style.display='block';
    document.getElementById('powerTypeli').style.display='block';
    break;


 }

}

</script> 
<title>Einstell.</title>
<meta http-equiv=Content-Type content="text/html; charset=utf-8" />
<meta name=viewport content="width=device-width, initial-scale=1.0">
</head>
<body class=settings-page>
$menu
<div style="font-size:32px;font-weight:bold;letter-spacing:2px;margin-bottom:10px;font-family:Abel,sans-serif">
<span id="esptime" onclick="window.location='/game'" style="cursor:pointer">--:--:--</span>
</div>
<div style="margin-bottom:20px">
<input type="time" id="mantime" step="1" style="font-size:18px;padding:8px;border-radius:3px;border:1px solid rgba(0,0,0,0.15)">
<button type="button" onclick="setManualTime()" style="font-size:14px;padding:9px 15px;background:#111;color:#fff;border:0;border-radius:3px;cursor:pointer;font-family:Abel,sans-serif;text-transform:uppercase;letter-spacing:1px">Zeit setzen</button>
</div>
<script>
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
</script>
<form class=form-verticle action=/settings method=GET>
<ul>
<li>
<label>Std-Marken:</label>
<div class=form-field>
<select name=hourmarks>
<option value=0 $hourmarks0>Keine</option>
<option value=1 $hourmarks1>Mittag</option>
<option value=2 $hourmarks2>Quadranten</option>
<option value=3 $hourmarks3>Stunden</option>
<option value=4 $hourmarks4>Abdunkeln</option>
</select>
</div>
</li>
<li class=checkbox>
<input id=showsecondshidden type=hidden name=showsecondshidden value=0>
<input id=showseconds type=checkbox name=showseconds $showseconds>
<label for=showseconds>Sekunden</label>
</li>
<li class=checkbox>
<input id=showsunpointhidden type=hidden name=showsunpointhidden value=0>
<input id=showsunpoint type=checkbox name=showsunpoint $showsunpoint>
<label for=showsunpoint>Sonnenpunkt <span data-tooltip="Zeigt die aktuelle Sonnenposition als goldenen LED-Punkt (nur tagsüber sichtbar, Breitengrad/Längengrad erforderlich)" class="tooltip" style="cursor:help;">&#9432;</span></label>
</li>
<li class=checkbox>
<input id=dawnbreakhidden type=hidden name=dawnbreakhidden value=0>
<input id=dawnbreak type=checkbox name=dawnbreak $dawnbreak>
<label for=dawnbreak>Sonnenaufgang <span data-tooltip="Simuliere den Sonnenaufgang eine Std. vor Einschalten der vollen Tagesuhrzeit" class="tooltip" style="cursor:help;">&#9432;</span></label>
</li>

<label data-tooltip="Nachts nur LED Punkte anzeigen" class = "tooltip section-head">Schlaf:</label>

<li id="rcorners2">
<label>Schlaf-Typ</label>
<div class=form-field>
<select name=sleeptype>
<option value=0 $sleeptype0>Schwarz</option>
<option value=1 $sleeptype1>Punkte</option>
<option value=2 $sleeptype2>Gedimmt</option>
<option value=3 $sleeptype3>Mondphase</option>
<option value=4 $sleeptype4>Aus</option>

</select>
</div>
<label data-tooltip="Helligkeit der Uhr im Schlafmodus (0=komplett aus, 100=volle Helligkeit)" class="tooltip">Nacht-Helligkeit</label>
<div class=form-field>
<input type=range name=nightbrightness min=0 max=100 value=$nightbrightness>
<span id=nbval>$nightbrightness%</span>
</div>
<script>
var nb=document.querySelector('input[name=nightbrightness]');
nb.oninput=function(){document.getElementById('nbval').innerText=this.value+'%';};
</script>
<label data-tooltip="Manuell: Feste Zeiten verwenden. Automatisch: Sonnenauf- und untergang berechnen (Breitengrad/Laengengrad erforderlich)" class="tooltip">Schlafsteuerung</label>
<div class=form-field>
<select name=autosleep id=autosleep onchange="toggleSleepFields()">
<option value=0 $autosleep0>Manuell</option>
<option value=1 $autosleep1>Automatisch (Sonne)</option>
</select>
</div>
<div id=manualsleep>
<label>Von</label>
<div class=form-field>

<input type=time name=sleep value=$sleep>
</div>

<label>Bis</label>
<div class=form-field>
<input type=time name=wake value=$wake>
</div>
</div>
</li>
<script>
function toggleSleepFields(){
var v=document.getElementById('autosleep').value;
document.getElementById('manualsleep').style.display=(v=='0')?'block':'none';
}
toggleSleepFields();
</script>
</p>
<li>
<label>Hemisphäre</label>
<div class=form-field>
<select name=hemisphere>
<option value=0 $hemisphere0>Nordhalbkugel</option>
<option value=1 $hemisphere1>Südhalbkugel</option>
</select>
</div>
</li>
<li>
<label>Zeitzone</label>
<div class=form-field>
<select name=timezone id=timezone>
<option value="1" $timezonevalue1 > (GMT-12:00) Internationale Datumsgrenze West</option>     
<option value="2" $timezonevalue2 > (GMT-11:00) Midwayinseln, Samoa</option>      
<option value="3" $timezonevalue3 > (GMT-10:00) Hawaii</option>        
<option value="4" $timezonevalue4 > (GMT-09:00) Alaska</option>        
<option value="5" $timezonevalue5 > (GMT-08:00) Pazifische Zeit (US & Kanada)</option>    
<option value="6" $timezonevalue6 > (GMT-08:00) Tijuana, Baja California</option>      
<option value="7" $timezonevalue7 > (GMT-07:00) Arizona</option>        
<option value="8" $timezonevalue8 > (GMT-07:00) Chihuahua, La Paz, Mazatlan</option>     
<option value="9" $timezonevalue9 > (GMT-07:00) Gebirgszeit (US & Kanada)</option>    
<option value="10" $timezonevalue10 > (GMT-06:00) Zentralamerika</option>       
<option value="11" $timezonevalue11 > (GMT-06:00) Zentrale Zeit (US & Kanada)</option>    
<option value="12" $timezonevalue12 > (GMT-06:00) Guadalajara, Mexico City, Monterrey</option>     
<option value="13" $timezonevalue13 > (GMT-06:00) Saskatchewan</option>        
<option value="14" $timezonevalue14 > (GMT-05:00) Bogota, Lima, Quito, Rio Branco</option>    
<option value="15" $timezonevalue15 > (GMT-05:00) Östliche Zeit (US & Kanada)</option>    
<option value="16" $timezonevalue16 > (GMT-05:00) Indiana (Ost)</option>       
<option value="17" $timezonevalue17 > (GMT-04:00) Atlantische Zeit (Kanada)</option>      
<option value="18" $timezonevalue18 > (GMT-04:00) Caracas, La Paz</option>      
<option value="19" $timezonevalue19 > (GMT-04:00) Manaus</option>        
<option value="20" $timezonevalue20 > (GMT-04:00) Santiago</option>        
<option value="21" $timezonevalue21 > (GMT-03:30) Newfoundland</option>        
<option value="22" $timezonevalue22 > (GMT-03:00) Brasilia</option>        
<option value="23" $timezonevalue23 > (GMT-03:00) Buenos Aires, Georgetown</option>      
<option value="24" $timezonevalue24 > (GMT-03:00) Greenland</option>        
<option value="25" $timezonevalue25 > (GMT-03:00) Montevideo</option>        
<option value="26" $timezonevalue26 > (GMT-02:00) Mittelatlantik</option>        
<option value="27" $timezonevalue27 > (GMT-01:00) Kapverdische Inseln</option>      
<option value="28" $timezonevalue28 > (GMT-01:00) Azores</option>        
<option value="29" $timezonevalue29 > (GMT+00:00) Casablanca, Monrovia, Reykjavik</option>      
<option value="30" $timezonevalue30 > (GMT+00:00) Greenwich: Dublin, Edinburgh, Lissabon, London</option> 
<option value="31" $timezonevalue31 > (GMT+01:00) Amsterdam, Berlin, Bern, Rome, Stockholm, Vienna</option>   
<option value="32" $timezonevalue32 > (GMT+01:00) Belgrade, Bratislava, Budapest, Ljubljana, Prague</option>    
<option value="33" $timezonevalue33 > (GMT+01:00) Brussels, Copenhagen, Madrid, Paris</option>     
<option value="34" $timezonevalue34 > (GMT+01:00) Sarajevo, Skopje, Warsaw, Zagreb</option>     
<option value="35" $timezonevalue35 > (GMT+01:00) Westliches Zentralafrika</option>      
<option value="36" $timezonevalue36 > (GMT+02:00) Amman</option>        
<option value="37" $timezonevalue37 > (GMT+02:00) Athens, Bucharest, Istanbul</option>      
<option value="38" $timezonevalue38 > (GMT+02:00) Beirut</option>        
<option value="39" $timezonevalue39 > (GMT+02:00) Cairo</option>        
<option value="40" $timezonevalue40 > (GMT+02:00) Harare, Pretoria</option>       
<option value="41" $timezonevalue41 > (GMT+02:00) Helsinki, Kyiv, Riga, Sofia, Tallinn, Vilnius</option>   
<option value="42" $timezonevalue42 > (GMT+02:00) Jerusalem</option>        
<option value="43" $timezonevalue43 > (GMT+02:00) Minsk</option>        
<option value="44" $timezonevalue44 > (GMT+02:00) Windhoek</option>        
<option value="45" $timezonevalue45 > (GMT+03:00) Kuwait, Riyadh, Baghdad</option>      
<option value="46" $timezonevalue46 > (GMT+03:00) Moscow, St. Petersburg, Volgograd</option>     
<option value="47" $timezonevalue47 > (GMT+03:00) Nairobi</option>        
<option value="48" $timezonevalue48 > (GMT+03:00) Tbilisi</option>        
<option value="49" $timezonevalue49 > (GMT+03:30) Tehran</option>        
<option value="50" $timezonevalue50 > (GMT+04:00) Abu Dhabi, Muscat</option>      
<option value="51" $timezonevalue51 > (GMT+04:00) Baku</option>        
<option value="52" $timezonevalue52 > (GMT+04:00) Yerevan</option>        
<option value="53" $timezonevalue53 > (GMT+04:30) Kabul</option>        
<option value="54" $timezonevalue54 > (GMT+05:00) Yekaterinburg</option>        
<option value="55" $timezonevalue55 > (GMT+05:00) Islamabad, Karachi, Tashkent</option>      
<option value="56" $timezonevalue56 > (GMT+05:30) Sri Jayawardenapura</option>       
<option value="57" $timezonevalue57 > (GMT+05:30) Chennai, Kolkata, Mumbai, New Delhi</option>    
<option value="58" $timezonevalue58 > (GMT+05:45) Kathmandu</option>        
<option value="59" $timezonevalue59 > (GMT+06:00) Almaty, Novosibirsk</option>       
<option value="60" $timezonevalue60 > (GMT+06:00) Astana, Dhaka</option>       
<option value="61" $timezonevalue61 > (GMT+06:30) Yangon (Rangoon)</option>       
<option value="62" $timezonevalue62 > (GMT+07:00) Bangkok, Hanoi, Jakarta</option>      
<option value="63" $timezonevalue63 > (GMT+07:00) Krasnoyarsk</option>        
<option value="64" $timezonevalue64 > (GMT+08:00) Beijing, Chongqing, Hong Kong, Urumqi</option>    
<option value="65" $timezonevalue65 > (GMT+08:00) Kuala Lumpur, Singapore</option>      
<option value="66" $timezonevalue66 > (GMT+08:00) Irkutsk, Ulaan Bataar</option>      
<option value="67" $timezonevalue67 > (GMT+08:00) Perth</option>        
<option value="68" $timezonevalue68 > (GMT+08:00) Taipei</option>        
<option value="69" $timezonevalue69 > (GMT+09:00) Osaka, Sapporo, Tokyo</option>      
<option value="70" $timezonevalue70 > (GMT+09:00) Seoul</option>        
<option value="71" $timezonevalue71 > (GMT+09:00) Yakutsk</option>        
<option value="72" $timezonevalue72 > (GMT+09:30) Adelaide</option>        
<option value="73" $timezonevalue73 > (GMT+09:30) Darwin</option>        
<option value="74" $timezonevalue74 > (GMT+10:00) Brisbane</option>        
<option value="75" $timezonevalue75 > (GMT+10:00) Canberra, Melbourne, Sydney</option>      
<option value="76" $timezonevalue76 > (GMT+10:00) Hobart</option>        
<option value="77" $timezonevalue77 > (GMT+10:00) Guam, Port Moresby</option>      
<option value="78" $timezonevalue78 > (GMT+10:00) Vladivostok</option>        
<option value="79" $timezonevalue79 > (GMT+11:00) Magadan, Solomon Is., New Caledonia</option>    
<option value="80" $timezonevalue80 > (GMT+12:00) Auckland, Wellington</option>       
<option value="81" $timezonevalue81 > (GMT+12:00) Fiji, Kamchatka, Marshall Is.</option>     
<option value="82" $timezonevalue82 > (GMT+13:00) Nuku'alofa</option>        

</select>
<br>
</li>
<li class=checkbox>
<input id=DSThidden type=hidden name=DSThidden value=0>
<input id=DST type=checkbox name=DST $DSTtime>
<label for=DST>Sommerzeit</label>
</li>
<li>
<label>Uhrentyp</label>
<div class=form-field>
<select name=clocktype id=clocktype onchange='CheckClockType(this.value);'>
      
<option value="1"  $original> Original</option>      
<option value="2"  $mini> Mini</option>        
<option value="3" $customtype > Benutzerdefiniert</option>      

</select>
<br>
</li>
<li id=pixelCountli style='display:$customvisible;'>
<input name=pixelCount id=pixelCount type="number" value=$pixelCount name="quantity" min="1" max="255">
<label for=pixelCount>Anzahl LEDs</label>
</li>
<li id=powerTypeli style='display:$customvisible;'>
<label>Strom</label>
<div class=form-field>
<select name=powerType id=powerType>
      
<option value="1" $maxbright255 > Netzstrom</option>      
<option value="2" $maxbright100 > USB-Strom</option>        
    

</select>

</li>


<li class=form-field>
<input id=clockname name=clockname value=$clockname type="text" autocorrect="off" autocapitalize="none">
<label data-tooltip="Netzwerkname der Uhr" class = "tooltip section-head" for=clockname>Uhrenname</label>
</div>

<label class=hide-mobile>&nbsp;</label>
<div class=btn-box>
<input class="btn btn-default" type=submit name=submit value='Speichern'/>
</div>
</ul>
</form>

</body>
</html>
)=====";
