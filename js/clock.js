var width; 
var height;
var needRedraw = false;
var key_left = false;
var key_right = false;
var key_up = false;
var drawRadius;
var innerRadius;
var curTime;
var lastSeconds = -1;
var TwoPI = 2 * Math.PI;
var pixelCount = 300;
var INTERVAL = TwoPI / pixelCount;
var ctx;// = $("#canvas")[0].getContext('2d');

var hourcolor;
var minutecolor;
var blendpoint = 0.5;
var clock = [];


function show(ctx, x, y) {
  ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (-Math.PI/2);

  for (var i = 0; i < pixelCount; i++) {
    var a = i * INTERVAL;
    var end = a + INTERVAL + (Math.PI / 100);
    
    ctx.beginPath ();
     ctx.fillStyle = clock[i].getCSSIntegerRGB();
     ctx.arc (0, 0, drawRadius, a, end);
     ctx.arc (0, 0, innerRadius, end, a, true);
     ctx.fill();
  };

  //make the black face
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius, 0, TwoPI);
  ctx.fillStyle = "black";
  ctx.fill();
  //make the centre bolt
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius*0.03, 0, TwoPI);
  ctx.fillStyle = "silver";
  ctx.fill();
  //ctx.restore();

  //make the white inner rim
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius, 0, TwoPI);
  ctx.strokeStyle = "white";
  ctx.stroke();
  //ctx.restore();
  //make the white outter rim
  ctx.beginPath ();
  ctx.arc (0,0, innerRadius*1.2, 0, TwoPI);
  //ctx.strokeStyle = "white";
  ctx.stroke();
  ctx.restore();




    ctx.beginPath();
    ctx.strokeStyle = "transparent";
    
    var grdRadial = ctx.createRadialGradient(x, y,innerRadius *1.2 , x, y, drawRadius);
    
    grdRadial.addColorStop(0, "rgba(255, 255, 255, 0)");//outside(1 is opauque)
    grdRadial.addColorStop(0.01, "rgba(255, 255, 255, 0.4)");//outside(1 is opauque)
    //grdRadial.addColorStop(0.501, "rgba(255, 255, 255, 0.2)");//outside(1 is opauque)
    //grdRadial.addColorStop(0.5, "rgba(255, 255, 255, 0)");
    grdRadial.addColorStop(1, "rgba(255, 255, 255, 1)"); //1 is outter (0 is transparent)
    
    
    //ctx.fillRect(10, 10, 150, 100);
    ctx.arc(x, y, drawRadius*1.01,0,2*Math.PI);
    ctx.fillStyle = grdRadial;
    ctx.fill();
}

function standardclock(hour_pos, min_pos, ctx, x, y) {
  ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (Math.PI);

  ctx.rotate (TwoPI*hour_pos/pixelCount);
  ctx.beginPath();
  ctx.moveTo(0,0);
  ctx.lineTo(0,innerRadius*0.6);
  ctx.strokeStyle = "white";
  ctx.lineWidth=4;
  ctx.stroke();
  ctx.restore();
    ctx.save();
  //make the face based on current colours loaded in mememory from epiphanyface
  ctx.translate (x, y);
  /* Make 0 degress = midnight */
  ctx.rotate (Math.PI);

  ctx.rotate (TwoPI*min_pos/pixelCount);
  ctx.beginPath();
  ctx.moveTo(0,0);
  ctx.lineTo(0,innerRadius*0.9);
  ctx.strokeStyle = "white";
  ctx.lineWidth=4;
  ctx.stroke();
  ctx.restore();

}


function parsergb(input) {
  m = input.match(/^rgb\s*\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)$/i);
    if( m) {
        return [m[1],m[2],m[3]];
    }

  }

var espHour = 0;
var espMinute = 0;
var espSecond = 0;
var espTimeLoaded = false;
var lastFetchMillis = 0;
var espShowSeconds = 1;
var espHourmarks = 0;
var espClockmode = 2;
var espBrightness = 100;
var espShowSunPoint = 0;
var espSunriseMinutes = 360;
var espSunsetMinutes = 1200;

function fetchESPState() {
  var x = new XMLHttpRequest();
  x.timeout = 3000;
  x.onreadystatechange = function() {
    if (x.readyState == 4 && x.status == 200) {
      try {
        var st = JSON.parse(x.responseText);
        espHour = st.h;
        espMinute = st.m;
        espSecond = st.s;
        espClockmode = st.clockmode;
        espShowSeconds = st.showseconds;
        espHourmarks = st.hourmarks;
        espBrightness = st.brightness;
        espShowSunPoint = st.showsunpoint;
        espSunriseMinutes = st.sunriseMinutes;
        espSunsetMinutes = st.sunsetMinutes;
        pixelCount = st.pixelCount;
        INTERVAL = TwoPI / pixelCount;
        hourcolor = new RGBColour(st.hourR, st.hourG, st.hourB);
        minutecolor = new RGBColour(st.minR, st.minG, st.minB);
        blendpoint = st.blendpoint / 255;
        document.getElementById("blendpoint").value = st.blendpoint;
        // Farb-Picker und hidden inputs mit Werten aus getstate aktualisieren
        if (st.hourcolor) {
          document.getElementById("hourcolor").value = st.hourcolor;
          document.getElementById("hourcolorspectrum").value = st.hourcolor;
          if ($("#hourcolorspectrum").spectrum) $("#hourcolorspectrum").spectrum("set", st.hourcolor);
        }
        if (st.minutecolor) {
          document.getElementById("minutecolor").value = st.minutecolor;
          document.getElementById("minutecolorspectrum").value = st.minutecolor;
          if ($("#minutecolorspectrum").spectrum) $("#minutecolorspectrum").spectrum("set", st.minutecolor);
        }
        // Brightness Slider
        if (document.getElementById("brightnessslider")) document.getElementById("brightnessslider").value = st.brightness;
        if (document.getElementById("brightness")) document.getElementById("brightness").innerHTML = st.brightness;
        // Firmware Version
        if (document.getElementById("fw-version")) document.getElementById("fw-version").innerHTML = st.fw;
        lastFetchMillis = Date.now();
        espTimeLoaded = true;
        needRedraw = true;
        if (clock.length != pixelCount) {
          clock = [];
          for (var i = 0; i < pixelCount; i++) {
            clock.push(new RGBColour(0, 0, 0));
          }
        }
      } catch(e) {}
    }
  };
  x.open("GET", "/getstate", true);
  x.send();
}

function showMidday() {
  var pos = 0;
  clock[pos] = new RGBColour(255, 255, 255);
}

function showQuadrants() {
  for (var i = 0; i < 4; i++) {
    var pos = Math.floor(i * pixelCount / 4);
    clock[pos] = new RGBColour(255, 255, 255);
  }
}

function showHourMarks() {
  for (var i = 0; i < 12; i++) {
    var pos = Math.floor(i * pixelCount / 12);
    clock[pos] = new RGBColour(255, 255, 255);
  }
}

function darkenToMidday(hour_pos, min_pos) {
  var midday = 0;
  var c = clock[midday].getIntegerRGB();
  clock[midday] = new RGBColour(Math.floor(c.r * 0.3), Math.floor(c.g * 0.3), Math.floor(c.b * 0.3));
}

function tick()
{

  var curHour, curMin, curSec;

  if (espTimeLoaded) {
    var elapsed = Math.floor((Date.now() - lastFetchMillis) / 1000);
    curSec = espSecond + elapsed;
    curMin = espMinute + Math.floor(curSec / 60);
    curSec = curSec % 60;
    curHour = espHour + Math.floor(curMin / 60);
    curMin = curMin % 60;
    curHour = curHour % 24;
  } else {
    var d = new Date();
    curHour = d.getHours();
    curMin = d.getMinutes();
    curSec = d.getSeconds();
  }

  var newTime = curSec;
  if (newTime != lastSeconds) {
    lastSeconds = newTime;
    needRedraw = true;
  }

  if (needRedraw) {
    needRedraw = false;

    ctx.clearRect(0,0,width,height);

    var hour_pos = Math.floor((curHour % 12) * pixelCount / 12 + curMin * pixelCount / 720);
    var min_pos = Math.floor(curMin * pixelCount / 60 + curSec * pixelCount / 3600);

    epiphanyface(hour_pos, min_pos);

    if (espHourmarks == 1) showMidday();
    if (espHourmarks == 2) showQuadrants();
    if (espHourmarks == 3) showHourMarks();
    if (espHourmarks == 4) darkenToMidday(hour_pos, min_pos);

    if (espShowSeconds) {
      var sec_pos = Math.floor(curSec * pixelCount / 60);
      clock[sec_pos] = new RGBColour(255, 255, 255);
    }

    if (espShowSunPoint) {
      var nowMinutes = curHour * 60 + curMin;
      if (nowMinutes >= espSunriseMinutes && nowMinutes <= espSunsetMinutes && espSunsetMinutes > espSunriseMinutes) {
        var sunProgress = (nowMinutes - espSunriseMinutes) / (espSunsetMinutes - espSunriseMinutes);
        var sun_pos = ((Math.floor(pixelCount / 4) - Math.floor(sunProgress * (pixelCount / 2))) + pixelCount) % pixelCount;
        clock[sun_pos] = new RGBColour(255, 180, 0);
      }
    }

    show(ctx, width/2, height/2);
  }
}


function epiphanyface(hour_pos,  min_pos)
{
//this face colours the clock in 2 sections, the c1->c2 divide represents the minute hand and the c2->c1 divide represents the hour hand.
      var c1;
      var c1blend;
      var c2;
      var c2blend;
      var gap;
      var firsthand = Math.min(hour_pos, min_pos);
      var secondhand = Math.max(hour_pos, min_pos);
    //check which hand is first, so we know what colour the 0 pixel is

    if(hour_pos>min_pos){       
        c2 = hourcolor;
        c1 = minutecolor;         
    }
    else
    {
        c1 = hourcolor;
        c2 = minutecolor;
    }

    c1blend = LinearBlend(c1, c2, blendpoint);

    
    c2blend = LinearBlend(c2, c1, blendpoint);


    gap = secondhand - firsthand;

    //create the blend between first and 2nd hand
    for(i=firsthand; i<secondhand; i++){
      clock[i] = LinearBlend(c2blend, c2, (i-firsthand)/gap);    
    }
    gap = pixelCount - gap;
    //and the last hand
    for(i=secondhand; i<pixelCount+firsthand; i++){
      clock[i%pixelCount] = LinearBlend(c1blend, c1, (i-secondhand)/gap);

    }
    //clock.SetPixelColor(hour_pos,hourcolor);
    //clock.SetPixelColor(min_pos,minutecolor);
}



function LinearBlend(left, right, progress)
{   
    var hue;
    var righth = right.getHSV().h;
    var lefth = left.getHSV().h;
    var d = righth-lefth;
    var temp;
    var output;
    var sat;
    var lum;

    //sat = left.getHSV().S + progress*(right.getHSV().S - left.getHSV().S);
    //lum = left.getHSV().L + progress*(right.getHSV().L - left.getHSV().L);

    sat = left.getHSV().s + ((right.getHSV().s - left.getHSV().s) * progress)
    lum = left.getHSV().v + ((right.getHSV().v - left.getHSV().v) * progress)

    if (left.getHSV().s==0||right.getHSV().s==0) // special case, one of the colours is white
    {
        if (left.getHSV().s==0)
        {
            hue = right.getHSV().h;
        } else {
            hue = left.getHSV().h;
        }
    } else {
    if (lefth > righth)
    {
        temp = righth;
        righth = lefth;
        lefth = temp;
        d = -d;
        progress = 1-progress;
    }

    if (d>180)
    {
        lefth += 360;

        hue = (lefth+progress*(righth-lefth));
        if (hue > 360)
        {
            hue -= 360;
        }
    }
    else {
        hue = lefth+progress*d;
    }
  }
    output = new HSVColour(hue, sat, lum);

    
    return output;
}

function manualmode() {
  document.getElementById("manualradio").checked = true;
}

function pad(n) {
  return (n < 10) ? ("0" + n) : n;
}

function middayIsTwelve(n) {
   var x;
    x = n;
    if (n == 0) {x = 12};
    if (n == 12) {x = 0};
    return x;
}

function parseCSScolour(color) {
    var digits = /(.*?)rgb\((\d+), (\d+), (\d+)\)/.exec(color);

    var red = parseInt(digits[2]);
    var green = parseInt(digits[3]);
    var blue = parseInt(digits[4]);

    var output = [red, green, blue];
    return output;
}

function parseHEXcolour(color) {
    var digits = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(color);

    var red = parseInt(digits[1], 16);
    var green = parseInt(digits[2], 16);
    var blue = parseInt(digits[3], 16);

    var output = [red, green, blue];
    return output;
}

document.addEventListener('DOMContentLoaded', function() {
  if (document.getElementById("canvas")) {

  ctx = document.getElementById("canvas").getContext('2d');

  // Startfarben - werden sofort durch fetchESPState ueberschrieben
  hourcolor = new RGBColour(255, 0, 0);
  minutecolor = new RGBColour(0, 0, 255);

  // Canvas auf volle Breite skalieren
  canvas.style.width ='100%';
  canvas.style.height='100%';
  canvas.width  = canvas.offsetWidth;
  canvas.height = canvas.width;

  width = document.getElementById("canvas").width;
  height = (width)+10;
  needRedraw = true;

  drawRadius = height/2 * 0.95;
  innerRadius = drawRadius * 0.5;
  fetchESPState();
  setInterval(fetchESPState, 5000);
  tick();
  setInterval(tick, 100);
  }
});
