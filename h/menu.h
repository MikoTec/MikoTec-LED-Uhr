const char menu_html[] PROGMEM = R"=====(
<button class="menu-btn" onclick="document.getElementById('menuOverlay').classList.toggle('open')" title="Menü">&#9776;</button>
<div class="menu-overlay" id="menuOverlay" onclick="if(event.target===this)this.classList.remove('open')">
  <div class="menu-panel">
    <a href="/">Hauptseite</a>
    <a href="/alarm">Alarm</a>
    <hr class="menu-sep">
    <a href="/settings">Einstellungen</a>
    <div class="menu-sub">
      <a href="/timezone">Zeitzone Manuell</a>
      <a href="/update" class="menu-green">Update</a>
    </div>
    <hr class="menu-sep">
    <a href="/hilfe">Hilfe</a>
    <div class="menu-sub">
      <a href="/support">Support</a>
      <a href="/cleareepromsure" class="menu-red">Werksreset</a>
      <span class="menu-green" onclick="if(confirm('Uhr wirklich neu starten?')){var x=new XMLHttpRequest();x.open('GET','/reboot',true);x.send();alert('Uhr wird neu gestartet...');}">Neustart</span>
    </div>
  </div>
</div>
)=====";
