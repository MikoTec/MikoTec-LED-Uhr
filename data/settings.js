function CheckClockType(val){
  var pli=document.getElementById('pixelCountli');
  var powi=document.getElementById('powerTypeli');
  var pixcount=document.getElementById('pixelCount');
  var power=document.getElementById('powerType');
  switch(val){
    case '1': pli.style.display='none'; powi.style.display='none'; pixcount.value=120; power.value=1; break;
    case '2': pli.style.display='none'; powi.style.display='none'; pixcount.value=60;  power.value=2; break;
    default:  pli.style.display='';    powi.style.display='';
  }
}

function toggleSleepFields(){
  var v=document.getElementById('autosleep').value;
  document.getElementById('manualsleep').style.display=(v=='0')?'block':'none';
}

function updateTime(){
  fetch('/getstate').then(r=>r.json()).then(function(d){
    var h=d.h||0,m=d.m||0,s=d.s||0;
    document.getElementById('esptime').innerText=
      (h<10?'0':'')+h+':'+(m<10?'0':'')+m+':'+(s<10?'0':'')+s;
  }).catch(function(){});
}

function setManualTime(){
  var t=document.getElementById('mantime').value;
  if(t==''){return;}
  fetch('/timeset?time='+t).then(function(){updateTime();}).catch(function(){});
}

document.addEventListener('DOMContentLoaded', function(){
  var nb=document.getElementById('nightbrightness');
  if(nb){nb.oninput=function(){document.getElementById('nbval').innerText=this.value+'%';};}

  fetch('/getsettings').then(r=>r.json()).then(function(d){
    if(d.hourmarks!=null) document.getElementById('hourmarks').value=d.hourmarks;
    document.getElementById('showseconds').checked  = (parseInt(d.showseconds)===1);
    document.getElementById('showsunpoint').checked = (parseInt(d.showsunpoint)===1);
    document.getElementById('dawnbreak').checked    = (parseInt(d.dawnbreak)===1);
    document.getElementById('DST').checked          = (parseInt(d.DSTtime)===1);
    if(d.sleeptype!=null) document.getElementById('sleeptype').value=d.sleeptype;
    if(d.nightbrightness!=null){
      document.getElementById('nightbrightness').value=d.nightbrightness;
      document.getElementById('nbval').innerText=d.nightbrightness+'%';
    }
    if(d.autosleep!=null){
      document.getElementById('autosleep').value=d.autosleep;
      toggleSleepFields();
    }
    if(d.sleep!=null&&d.sleepmin!=null){
      var sh=(d.sleep<10?'0':'')+d.sleep;
      var sm=(d.sleepmin<10?'0':'')+d.sleepmin;
      document.getElementById('sleep').value=sh+':'+sm;
    }
    if(d.wake!=null&&d.wakemin!=null){
      var wh=(d.wake<10?'0':'')+d.wake;
      var wm=(d.wakemin<10?'0':'')+d.wakemin;
      document.getElementById('wake').value=wh+':'+wm;
    }
    if(d.hemisphere!=null)   document.getElementById('hemisphere').value=d.hemisphere;
    if(d.timezonevalue!=null) document.getElementById('timezone').value=d.timezonevalue;
    if(d.pixelCount!=null&&d.maxBrightness!=null){
      if(d.pixelCount==120&&d.maxBrightness==255){
        document.getElementById('clocktype').value='1';
        document.getElementById('pixelCountli').style.display='none';
        document.getElementById('powerTypeli').style.display='none';
        document.getElementById('pixelCount').value=120;
        document.getElementById('powerType').value=1;
      } else if(d.pixelCount==60&&d.maxBrightness==100){
        document.getElementById('clocktype').value='2';
        document.getElementById('pixelCountli').style.display='none';
        document.getElementById('powerTypeli').style.display='none';
        document.getElementById('pixelCount').value=60;
        document.getElementById('powerType').value=2;
      } else {
        document.getElementById('clocktype').value='3';
        document.getElementById('pixelCountli').style.display='';
        document.getElementById('powerTypeli').style.display='';
        document.getElementById('pixelCount').value=d.pixelCount;
        document.getElementById('powerType').value=(d.maxBrightness==255)?1:2;
      }
    }
    if(d.clockname!=null) document.getElementById('clockname').value=d.clockname;
  }).catch(function(e){console.error('getsettings fehler:',e);});

  setInterval(updateTime,1000);
  updateTime();
});
