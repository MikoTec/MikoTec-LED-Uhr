// Einstellungen JavaScript

function CheckClockType(val) {
  var pixcount = document.getElementById('pixelCount');
  var power = document.getElementById('maxbright');
  switch(val) {
    case '1':
      document.getElementById('pixelCountli').style.display='none';
      document.getElementById('powerTypeli').style.display='none';
      pixcount.value=120; power.value=255; break;
    case '2':
      document.getElementById('pixelCountli').style.display='none';
      document.getElementById('powerTypeli').style.display='none';
      pixcount.value=60; power.value=100; break;
    default:
      document.getElementById('pixelCountli').style.display='';
      document.getElementById('powerTypeli').style.display='';
  }
}

// Zeit per getstate aktualisieren
function updateTime() {
  fetch('/getstate').then(r=>r.json()).then(function(d){
    var h=d.h||0, m=d.m||0, s=d.s||0;
    document.getElementById('esptime').innerText =
      (h<10?'0':'')+h+':'+(m<10?'0':'')+m+':'+(s<10?'0':'')+s;
    document.getElementById('fwver').innerText = 'Version ' + (d.fw||'--');
  }).catch(function(){});
}

// Einstellungen laden und Formular befüllen
fetch('/getsettings').then(r=>r.json()).then(function(d){
  if(d.pixelCount) document.getElementById('pixelCount').value = d.pixelCount;
  if(d.clockname)  document.getElementById('clockname').value  = d.clockname;
  if(d.hourmarks != null) document.getElementById('hourmarks').value = d.hourmarks;
  if(d.sleeptype != null) document.getElementById('sleeptype').value = d.sleeptype;
  if(d.sleep != null) document.getElementById('sleep').value = d.sleep;
  if(d.sleepmin != null) document.getElementById('sleepmin').value = d.sleepmin;
  if(d.wake != null) document.getElementById('wake').value = d.wake;
  if(d.wakemin != null) document.getElementById('wakemin').value = d.wakemin;
  if(d.nightbrightness != null) document.getElementById('nightbrightness').value = d.nightbrightness;
  if(d.hemisphere != null) document.getElementById('hemisphere').value = d.hemisphere;
  if(d.DSTauto != null) document.getElementById('DSTauto').value = d.DSTauto;
  if(d.showseconds) document.getElementById('showseconds').checked = d.showseconds == 1;
  if(d.showsunpoint != null) document.getElementById('showsunpoint').checked = d.showsunpoint == 1;
  if(d.dawnbreak != null) document.getElementById('dawnbreak').checked = d.dawnbreak == 1;
  if(d.autosleep != null) document.getElementById('autosleep').checked = d.autosleep == 1;
  if(d.maxBrightness) document.getElementById('maxbright').value = d.maxBrightness;
}).catch(function(){});

setInterval(updateTime, 1000);
updateTime();
