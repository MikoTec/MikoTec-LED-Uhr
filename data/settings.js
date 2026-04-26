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

function toggleSleepFields() {
  var v = document.getElementById('autosleep').value;
  document.getElementById('manualsleep').style.display = (v == '0') ? 'block' : 'none';
}

// Nacht-Helligkeit Slider Live-Anzeige
var nb = document.getElementById('nightbrightness');
if (nb) {
  nb.oninput = function() { document.getElementById('nbval').innerText = this.value + '%'; };
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
  if(d.nightbrightness != null) {
    document.getElementById('nightbrightness').value = d.nightbrightness;
    document.getElementById('nbval').innerText = d.nightbrightness + '%';
  }
  // sleep/wake als time-Felder: "HH:MM"
  if(d.sleep != null && d.sleepmin != null) {
    var sh = (d.sleep < 10 ? '0' : '') + d.sleep;
    var sm = (d.sleepmin < 10 ? '0' : '') + d.sleepmin;
    document.getElementById('sleep').value = sh + ':' + sm;
  }
  if(d.wake != null && d.wakemin != null) {
    var wh = (d.wake < 10 ? '0' : '') + d.wake;
    var wm = (d.wakemin < 10 ? '0' : '') + d.wakemin;
    document.getElementById('wake').value = wh + ':' + wm;
  }
  if(d.hemisphere != null) document.getElementById('hemisphere').value = d.hemisphere;
  if(d.DSTauto != null) document.getElementById('DSTauto').value = d.DSTauto;
  if(d.showseconds != null) document.getElementById('showseconds').checked = d.showseconds == 1;
  if(d.showsunpoint != null) document.getElementById('showsunpoint').checked = d.showsunpoint == 1;
  if(d.dawnbreak != null) document.getElementById('dawnbreak').checked = d.dawnbreak == 1;
  if(d.autosleep != null) {
    document.getElementById('autosleep').value = d.autosleep;
    toggleSleepFields();
  }
  if(d.maxBrightness != null) {
    document.getElementById('maxbright').value = d.maxBrightness;
    // clocktype ableiten
    if(d.pixelCount == 120 && d.maxBrightness == 255) {
      document.getElementById('clocktype').value = '1';
      document.getElementById('pixelCountli').style.display='none';
      document.getElementById('powerTypeli').style.display='none';
    } else if(d.pixelCount == 60 && d.maxBrightness == 100) {
      document.getElementById('clocktype').value = '2';
      document.getElementById('pixelCountli').style.display='none';
      document.getElementById('powerTypeli').style.display='none';
    }
  }
}).catch(function(){});

setInterval(updateTime, 1000);
updateTime();
toggleSleepFields();
